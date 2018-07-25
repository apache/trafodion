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
* File:         GenItemExpr.C
* Description:  Item expressions
*
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "AllItemExpr.h"
#include "GenExpGenerator.h"
#include "dfs2rec.h"
#include "exp_clause.h"
#include "exp_clause_derived.h"
#include "NumericType.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   Aggregate::codegen_and_set_attributes()
//   Aggregate::codeGen()
//   Assign::codeGen()
//   BaseColumn::codeGen()
//   BiArith::codeGen()
//   BiLogic::codeGen()
//   BiRelat::codeGen()
//   ConstValue::codeGen()
//   Convert::codeGen()
//   DynamicParam::codeGen()
//   HostVar::codeGen()
//   IndexColumn::codeGen()
//   ItemExpr::codeGen()
//   ItemExpr::codegen_and_set_attributes()
//   ItemList::codeGen()
//   UnLogic::codeGen()
//   ValueIdRef::codeGen()
//   ValueIdUnion::codeGen()
//
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// The following two functions are static functions used during generation of
// certain aggregate expressions, in particular ITM_ONE_ROW aggregate. They are
// helper functions used by Aggregate::codeGen() and
// Aggregate::code_gen_set_attributes()
////////////////////////////////////////////////////////////////////////////
void findnumleaves( ItemExpr *exp, Int32& degree )
{
  for (short i=0; i<exp->getArity(); i++)
  {
    if (exp->child(i)->getOperatorType() != ITM_ITEM_LIST)
       ++degree;
    else
       findnumleaves(exp->child(i)->castToItemExpr(), degree);
  }
} // findnumleaves()

void collectAttributes( ItemExpr *exp, Lng32 degree,
                        Int32& counter, Attributes **attr,
                        Generator * generator)
{
  GenAssert((counter <= degree), "Row attributes not set properly");

  for (short i=0; i<exp->getArity(); i++)
  {
    if (exp->child(i)->getOperatorType() != ITM_ITEM_LIST)
    {
       ++counter;
       attr[counter] = generator->getMapInfo(exp->child(i)->
                                      castToItemExpr()->getValueId())
                         ->getAttr();
    }
    else
      collectAttributes( exp->child(i)->castToItemExpr(), degree,
                         counter, attr, generator );
  }

} // collectAttributes()
void ItemExpr::codegen_and_set_attributes(Generator * generator, 
					  Attributes **attr,
                                          Lng32 num_attrs)
{
  for (short i=0; i<num_attrs-1; i++) // here num_attrs-1 refers to Arity()
  {
    if (! child(i))
      continue;

    generator->getExpGenerator()->setClauseLinked(FALSE);
    // Needs revalidation
    if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON)
    {
      if (child(i)->getOperatorType() == ITM_RANGE_SPEC_FUNC)
      {
	child(i) = child(i)->child(1);
      }
    }
    child(i)->codeGen(generator);
    attr[i+1] = generator->getAttr(child(i));
    generator->getExpGenerator()->setClauseLinked(FALSE);
  }
} // ItemExpr::codegen_and_set_attributes()

void Aggregate::codegen_and_set_attributes( Generator * generator, Attributes **attr,
                                            Lng32 num_attrs )
{

  if ( getOperatorType() != ITM_ONE_ROW )
  {
    ItemExpr::codegen_and_set_attributes( generator, attr, num_attrs);
  }
  else
  {
    MapTable *map_table = generator->getMapTable();

    for (short i=0; i<getArity(); i++)
    {
      child(i)->codeGen(generator);
    }
    // collect attributes at leaves
    // note that variable num_attrs gives the number of leaves,
    // but not the arity

    Int32 counter = 0;
    collectAttributes( this, num_attrs, counter, attr, generator );
  }
} // Aggregate::codegen_and_set_attributes()

short Aggregate::codeGen(Generator * generator)
{
  Attributes ** attr;

  // If this Aggr has already been codeGenned, then bug out early.
  //
  MapInfo * aggrMapInfo = generator->getMapInfoAsIs(getValueId());
  if (aggrMapInfo && aggrMapInfo->isCodeGenerated())
    return 0;

  if (getOperatorType() != ITM_ONE_ROW)
  {
    if (generator->getExpGenerator()->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
      return 0;
  }

  ex_clause * clause = 0;
  
  switch (getOperatorType())
    {
    case ITM_ONE_ROW:
      {
        Int32 degree = 0;
        findnumleaves(this, degree); // degree has number of leaves in the tree

        if (generator->getExpGenerator()->genItemExpr(this, &attr, (1+degree),
                                                               -1) == 1)
          return 0;

	clause =
	  new(generator->getSpace()) ex_aggr_one_row_clause(getOperatorType(),
							    (short)(1+degree), 
							    attr,
							    generator->getSpace());
      }
      break;

    case ITM_ANY_TRUE_MAX:
      {
	clause =
	  new(generator->getSpace()) ex_aggr_any_true_max_clause(getOperatorType(),
								 (short)(1+getArity()), 
								 attr, generator->getSpace());
      }
      break;
      
    default:
      break;
      
    }

  GenAssert(clause, "Aggregate::codeGen -- missing clause!");
  generator->getExpGenerator()->linkClause(this, clause);  

  return 0;
}


short Assign::codeGen(Generator * generator)
{
  Attributes ** attr;

  // If this Assign has already been codeGenned, then bug out early.
  //
  MapInfo * assignMapInfo = generator->getMapInfoAsIs(getValueId());
  if (assignMapInfo && assignMapInfo->isCodeGenerated())
    return 0;

  // If the left child (lvalue) is already in the map table, then
  // add the Assign value Id to the map table with the same attributes
  // as the let child. Mark the Assign node as codeGenned. Also, allocate
  // space for the attributes.
  //
  MapInfo *leftChildMapInfo = generator->getMapInfoAsIs
    (child(0)->castToItemExpr()->getValueId());
  if (leftChildMapInfo)
    {
      if (! assignMapInfo)
	assignMapInfo = 
	  generator->addMapInfoToThis(generator->getLastMapTable(), 
				      getValueId(), 
				      leftChildMapInfo->getAttr());
      assignMapInfo->codeGenerated();
      attr = new(generator->wHeap()) Attributes*[2];

      // Set the result attribute
      //
      attr[0] = assignMapInfo->getAttr();
    }
  // Otherwise, go ahead and generate the Assign attributes (which also
  // allocates space for the Assign result). Add the left child to the
  // map table with the same attributes as the Assign node.
  //
  else
    {
      generator->getExpGenerator()->genItemExpr(this, &attr, 2, 0);
      generator->addMapInfoToThis(generator->getLastMapTable(), 
				  child(0)->castToItemExpr()->getValueId(),
				  attr[0]);
    }

  attr[0]->resetShowplan();

  // Now, generate code for the right child (rvalue).
  //
  generator->getExpGenerator()->setClauseLinked(FALSE);
  child(1)->codeGen(generator);
  attr[1] = generator->getAttr(child(1));
  generator->getExpGenerator()->setClauseLinked(FALSE);

  ex_conv_clause * conv_clause =
    new(generator->getSpace()) ex_conv_clause
    (getOperatorType(), attr, generator->getSpace());
  generator->getExpGenerator()->linkClause(this, conv_clause);

  return 0;
}

short BaseColumn::codeGen(Generator * generator)
{
  // if this column has not been added to the map table or
  // it does not have a valid offset, and
  // it has an EIC list, then add it to the map table and
  // assign attributes from the one of the EIC values.
  MapInfo * mapInfo = generator->getMapInfoAsIs(getValueId());
  if ((!mapInfo ||
       !mapInfo->isOffsetAssigned()) &&
      (!getEIC().isEmpty()))
    {
      MapInfo * index_col_map_info;
      short done = 0;
      for (ValueId val_id = getEIC().init();
	   !done && getEIC().next(val_id);
	   getEIC().advance(val_id))
	{
	  if (index_col_map_info = generator->getMapInfoAsIs(val_id))
	    {
	      Attributes * attr = 
		generator->addMapInfo(getValueId(), 0)->getAttr();
	      attr->copyLocationAttrs(index_col_map_info->getAttr());
	      done = -1;
	    }
	}
    }
      
  return 0;
}

short BiArith::codeGen(Generator * generator)
{
  Attributes ** attr;
  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;

  // if temp space is needed for this operation, set it.
  if (attr[0]->isComplexType())
    {
      eg->addTempsLength(((ComplexType *)attr[0])->setTempSpaceInfo(getOperatorType(),
								    eg->getTempsLength()));
    }
			      
  attr[0]->resetlastdaymonthflag();
  attr[0]->resetlastdayonerrflag();

// Check to see which type of rounding is needed for add_months, date_add
// functions.  Set flags here for use in executor datetime.cpp.
  if (isStandardNormalization())
     attr[0]->setlastdayonerrflag();
  if (isKeepLastDay())
     attr[0]->setlastdaymonthflag();

  ex_arith_clause * arith_clause =
    new(generator->getSpace()) 
    ex_arith_clause(getOperatorType(), attr, generator->getSpace(),
		    (short)getRoundingMode(),
		    getDivToDownscale());

  generator->getExpGenerator()->linkClause(this, arith_clause);

  return 0;
}


short BiArithSum::codeGen(Generator * generator)
{
  Attributes ** attr;
  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;

  // if temp space is needed for this operation, set it.
  if (attr[0]->isComplexType())
    {
      eg->addTempsLength(((ComplexType *)attr[0])->setTempSpaceInfo(getOperatorType(),
								    eg->getTempsLength()));
    }
			      
  ex_arith_sum_clause * arith_clause =
    new(generator->getSpace()) 
    ex_arith_sum_clause(getOperatorType(), 
			attr, 
			generator->getSpace());
  if (eg->inSequenceFuncExpr())
     arith_clause->setAugmentedAssignOperation(FALSE);
  
  generator->getExpGenerator()->linkClause(this, arith_clause);

  return 0;
}


short BiArithCount::codeGen(Generator * generator)
{
  Attributes ** attr;
  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;

  // if temp space is needed for this operation, set it.
  if (attr[0]->isComplexType())
    {
      eg->addTempsLength(((ComplexType *)attr[0])->setTempSpaceInfo(getOperatorType(),
								    eg->getTempsLength()));
    }
			      
  ex_arith_count_clause * arith_clause =
    new(generator->getSpace()) 
    ex_arith_count_clause(getOperatorType(), 
			  attr, 
			  generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, arith_clause);

  return 0;
}

short UnArith::codeGen(Generator * generator)
{
  Attributes ** attr;
  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;


  ex_arith_clause * arith_clause = 
    new(generator->getSpace()) 
    ex_arith_clause(getOperatorType(), attr, generator->getSpace(),
                    0, FALSE);
  
  generator->getExpGenerator()->linkClause(this, arith_clause);

  return 0;
}

void
markGeneratedEntries(Generator *generator, ItemExpr *item, ValueIdSet &marks)
{
  if(item) {
    MapInfo *mapInfo =
      generator->getMapInfoAsIs(item->getValueId());
    
    if(mapInfo && mapInfo->isCodeGenerated())
      marks += item->getValueId();

    for(Int32 i = 0; i < item->getArity(); i++) {
      markGeneratedEntries(generator,item->child(i), marks);
    }
  }
}

void
generateMarkedEntries(Generator *generator, ValueIdSet &marks)
{

  for(ValueId vid = marks.init(); marks.next(vid); marks.advance(vid)) {
    MapInfo *mapInfo =
      generator->getMapInfoAsIs(vid);
    if(mapInfo)
      mapInfo->codeGenerated();
  }
}

void
unGenerate(Generator *generator, ItemExpr *item)
{
  if(item) {
    MapInfo *mapInfo =
      generator->getMapInfoAsIs(item->getValueId());
    
    if(mapInfo) 
      mapInfo->resetCodeGenerated();
    
    for(Int32 i = 0; i < item->getArity(); i++) {
      unGenerate(generator,item->child(i));
    }
  }
}

short BiLogic::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1+getArity()), 0) == 1)
    return 0;
    
  Space * space = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // Normally, if code for a value id has been generated, and if
  // that value id is seen again, then code is not generated. The
  // location where the result is available is returned instead.
  // The case of a logical operator is different. Code is generated
  // again if a value id from the left child is also present in
  // the right child. This is done 
  // because at expression evaluation time, some of the expressions
  // may be skipped due to short circuit evaluation. 
  //
  // Allocate a new map table before generating code for each child.
  // This map table contains all the temporary results produced by
  // the child. 
  // Remove this map table after generating code for each child.
  generator->appendAtEnd();
  expGen->incrementLevel();

  codegen_and_set_attributes(generator, attr, 2); 
  //  generator->getExpGenerator()->setClauseLinked(FALSE);
  // child(0)->codeGen(generator);
  // attr[1] = generator->getAttr(child(0));
  // generator->getExpGenerator()->setClauseLinked(FALSE);

  /* generate boolean short circuit code */
  Attributes ** branch_attr = new(generator->wHeap()) Attributes * [2];

  branch_attr[0] = attr[0]->newCopy(generator->wHeap());
  branch_attr[0]->copyLocationAttrs(attr[0]);

  branch_attr[1] = attr[1]->newCopy(generator->wHeap());
  branch_attr[1]->copyLocationAttrs(attr[1]);

  branch_attr[0]->resetShowplan();
  ex_branch_clause * branch_clause 
    = new(space) ex_branch_clause(getOperatorType(), branch_attr, space);
  
  generator->getExpGenerator()->linkClause(0, branch_clause);
 
  generator->removeLast();
  expGen->decrementLevel();
  generator->appendAtEnd();
  expGen->incrementLevel();

  ValueIdSet markedEntries;
  // This ia a MapTable entry related fix for RangeSpec transformation.
  if( child(1)->getOperatorType() == ITM_RANGE_SPEC_FUNC )
    markGeneratedEntries(generator, child(1)->child(1), markedEntries);
  else
    markGeneratedEntries(generator, child(1), markedEntries);
//  if( child(1)->getOperatorType() == ITM_RANGE_SPEC_FUNC )
//    child(1)->child(1)->codeGen(generator);
//  else
    child(1)->codeGen(generator);
  ItemExpr *rightMost;
  if( child(1)->getOperatorType() == ITM_RANGE_SPEC_FUNC )
    rightMost = child(1)->child(1)->castToItemExpr();
  else
    rightMost = child(1)->castToItemExpr();

  while (rightMost->getOperatorType() == ITM_ITEM_LIST)
    rightMost = rightMost->child(1)->castToItemExpr();

  attr[2] = generator->
    getMapInfo(rightMost->getValueId())->getAttr();

  ex_bool_clause * bool_clause =
    new(space) ex_bool_clause(getOperatorType(), attr, space);

  generator->getExpGenerator()->linkClause(this, bool_clause);

  branch_clause->set_branch_clause((ex_clause *)bool_clause);
  
  generator->removeLast();
  expGen->decrementLevel();
  if( child(1)->getOperatorType() == ITM_RANGE_SPEC_FUNC )
    unGenerate(generator, child(1)->child(1));
  else
    unGenerate(generator, child(1));
  generateMarkedEntries(generator, markedEntries);

  return 0;
}

short BiRelat::codeGen(Generator * generator)
{
  if (child(0)->getOperatorType() == ITM_ITEM_LIST)
    {
      GenAssert(0, "Multivalued predicated should have been converted in preCodeGen");
    }
  else
    {
      Attributes ** attr;
      
      if (generator->getExpGenerator()->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
	return 0;
      
      ex_comp_clause * comp_clause =
	new(generator->getSpace()) ex_comp_clause(getOperatorType(), attr,
						  generator->getSpace(),
						  getSpecialNulls());

      comp_clause->setCollationEncodeComp(getCollationEncodeComp());
      
      if (rollupColumnNum() >= 0)
        comp_clause->setRollupColumnNum(rollupColumnNum());

      generator->getExpGenerator()->linkClause(this, comp_clause);
    }
  
  return 0;
}


short ConstValue::codeGen(Generator * generator)
{
  MapInfo * map_info = generator->getMapInfoAsIs(getValueId());
  if (map_info && map_info->isCodeGenerated())
    return 0;

  if (!map_info)
    {
      // this value has not been added to the map table. Add it.
      // Constants are added to the root Maptable of the expression
      // being generated.
      // This is done because the last map table may be removed to
      // get rid of temps (like, if BiLogic is being generated: 
      // see BiLogic::codeGen)
      // but we don't want to remove the constants until after the
      // whole expression has been generated.
      map_info = 
	generator->addMapInfoToThis(generator->getExpGenerator()->getExprMapTable(), getValueId(), NULL);

      Attributes * map_attr = map_info->getAttr();
      map_attr->setAtp(0);
      map_attr->setAtpIndex(0);

      // compute length of this temp and assign offsets to map_attr.
      // All temps are in sqlark_exploded format.
      ULng32 len;
      ExpTupleDesc::computeOffsets(map_attr, 
				   ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				   len, 
				   generator->getExpGenerator()->getConstLength()); // start here
      generator->getExpGenerator()->addConstLength(len);
    }

  map_info->codeGenerated();

  generator->getExpGenerator()->linkConstant(this);
  
  return 0;
}

short Convert::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_conv_clause * conv_clause =
	  new(generator->getSpace()) ex_conv_clause(getOperatorType(), attr,
						    generator->getSpace());
  conv_clause->setLastVOAoffset(lastVOAOffset_);
  conv_clause->setLastNullIndicatorLength(lastNullIndicatorLength_);
  conv_clause->setLastVcIndicatorLength(lastVcIndicatorLength_);
  conv_clause->setAlignment(alignment_);
  

  generator->getExpGenerator()->linkClause(this, conv_clause);      
  
  return 0;
}

short Parameter::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  return 0;
}


short HostVar::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  return 0;
}

short IndexColumn::codeGen(Generator * /*generator*/)
{
  return 0;
}

short ItemExpr::codeGen(Generator * generator)
{
  if (getOperatorType() == ITM_NATYPE ||
      getOperatorType() == ITM_NAMED_TYPE_TO_ITEM)
    {
      Attributes ** attr;
      
      if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
	return 0;
      return 0;
    }
  
  NAString txt(getText());
  txt += " should never reach ItemExpr::codeGen";
  GenAssert(0, txt);
  return -1;
}

short ItemList::codeGen(Generator * generator)
{
  for (short i=0; i<getArity(); i++)
    {
      child(i)->codeGen(generator);
    }

  return 0;
}

short UnLogic::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, 
						(1 + getArity()), -1) == 1)
    return 0;

  ex_unlogic_clause * unlogic_clause =
    new(generator->getSpace()) ex_unlogic_clause(getOperatorType(), attr,
						 generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, unlogic_clause);
  
  return 0;
}


short ValueIdRef::codeGen(Generator * generator)
{
  MapInfo * map_info = generator->getMapInfoAsIs(getValueId());
  if (!map_info)
    {
      /* this value has not been added to the map table. Add it.*/
      map_info = 
	generator->addMapInfo(getValueId(), 
			      generator->getMapInfo(isDerivedFrom())->getAttr());
      
    }
  
  return 0;
}

short ValueIdUnion::codeGen(Generator * /*generator*/)
{
  return 0;
}
