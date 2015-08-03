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
#include "Generator.h"
#include "GenExpGenerator.h"
#include "exp_function.h"
#include "exp_math_func.h"
#include "CharType.h"
#include "NumericType.h"
#include "ItemSample.h"
#include "ItmFlowControlFunction.h"

// ItmBalance::preCodeGen
//
// Generate the code to implmenent FIRSTN, PERIODIC, and RANDOM sampling
// for this branch of a balance node tree.
//
ItemExpr *ItmBalance::preCodeGen(Generator *generator)
{
  // Get a local handle on some things...
  //
  ExpGenerator *expGen = generator->getExpGenerator();
  CollHeap *wHeap = generator->wHeap();

  // For both FIRSTN and PERIODIC sampling, allocate an integer index
  // counter and an expression to increment the counter.
  //
  ItemExpr *counterExpr = NULL, *incrementExpr = NULL;
  if((sampleType() == RelSample::FIRSTN) ||
     (sampleType() == RelSample::PERIODIC))
    {
      counterExpr = new(wHeap) ItmPersistentExpressionVar(0);
      incrementExpr = new(wHeap) ItmBlockFunction
	(counterExpr,
	 new(wHeap) Assign(counterExpr,
			   new(wHeap) BiArith(ITM_PLUS, 
					      counterExpr, 
					      new (wHeap) ConstValue(1))));
    }
					      
  // Create the expression to evaluate the <sample condition> for this
  // branch of the balance clause. The sample condition returns an integer
  // indicating the number of times to return the current row or -1 if 
  // sampling if completely finished. The integer is typically 0 or 1.
  //
  // Aftet the sample condition expression is created, it is combined with
  // any child balance branches to create an expression for the entire
  // balance subtree rooted at this node.
  //
  ItemExpr *sampleCondition = NULL;

  // First N sampling.
  //
  // Initialization: 
  //  Counter = 0;
  // Per Row: 
  //  if Counter < SampleSize then Counter++, 1; else 0;
  //
  if(sampleType() == RelSample::FIRSTN)
    {
      // Create the expression Counter++, 1;
      //
      ItemExpr *conditionTrue = new(wHeap) ItmBlockFunction
	(incrementExpr,
	 new(wHeap) ConstValue(1));


      // Construct the expression used when we are done with this
      // FIRSTN counter.
      //
      // Returning 0, causes sample to continue to process rows 
      // Returning -1 causes sample to cancel the request.
      //
      // We would like to cancel the request when we have satisfied
      // the FIRSTN.  However, If there are nested FIRSTN counters, we
      // must wait for all counters to complete before returning -1.
      // For now just punt and return 0 in the case of nested FIRSTN.
      // This will cause the scan and sample to continue processing
      // rows, even after the sample is complete. At some point we
      // should extend this expression to account for this case.
      // Maybe have the doneValue be something like:
      //
      // (case when (counter1 >= sampleSize1 and counter2 >= sampleSize2 and ...)
      //       then -1
      //       else 0
      //  end)
      //
      // The problem is that we do not have a handle on all the
      // counters and sizes at this point.
      //
      ItemExpr *doneValue;
      if (getNextBalance() || generator->inNestedFIRSTNExpr()) {

        // set flag in generator to indicate to Balance expressions
        // below that we are in a nested balance expression.  Need
        // this flag since the leaf balance expression will not have a
        // nextBalance expression and will not know that it is nested.
        //
        generator->setInNestedFIRSTNExpr(TRUE);

        // Punt and just allow the scan/sample to continue in the case
        // of nested FISRTN sampling.
        //
        doneValue = new(wHeap) ConstValue(0);
      } else {
        
        // Cause the Sample to cancel the request and stop processing
        // rows.
        //
        doneValue = new(wHeap) ConstValue(-1);
      }

      // Create the remainder of the expression.
      //
      sampleCondition = expGen->createExprTree
	("CASE WHEN @A1 < @A2 THEN @A3 ELSE @A4 END",
	 0, 4,
	 counterExpr,              // row counter
	 getSampleSize(),          // sample size
	 conditionTrue,            // expression if row qualifies
         doneValue                 // expression if we are done with
                                   // this FIRSTN
	 );
    }
  // Periodic sampling.
  //
  // Initialization: 
  //  Counter = 0;
  // Per Row: 
  //  Counter++, if Counter <= 0 then 0; else if(Counter <= sampleSize) 1;
  //             else Counter = - samplePeriod, 0;
  else if(sampleType() == RelSample::PERIODIC)
    {
      // Compute - samplePeriod.
      //
      ItemExpr *sampleSkip = new(wHeap) BiArith(ITM_MINUS,
						getSampleSize(),
						getSkipSize());

      // Create the expression Counter = sampleSkip, 1;
      //
      ItemExpr *resetExpr = new(wHeap) ItmBlockFunction
	(new(wHeap) Assign(counterExpr, sampleSkip),
	 new(wHeap) ConstValue(1));

      // Create the remainder of the expression.
      //
      ItemExpr *caseExpr = expGen->createExprTree
	("CASE WHEN @A1 <= 0 THEN 0 "
	 "     WHEN @A1 < @A2 THEN 1 "
	 "     WHEN @A2 > 0 THEN @A3 " 
	 "     ELSE 0 END",
	 0, 4,
	 counterExpr,    // row counter
	 getSampleSize(),// sample size
	 resetExpr       // reset expression and return 1
	 );  

      // Always increment the counter, then apply the case expression
      // and returns it's result.
      //
      sampleCondition = new(wHeap) ItmBlockFunction
	(incrementExpr, caseExpr);
    }
  else if(sampleType() == RelSample::RANDOM)
    {
      CMPASSERT(!isAbsolute());
      NABoolean negate = FALSE;
      ConstValue *sizeExpr = (child(1)
                          ? child(1)->castToConstValue(negate)
                          : NULL);
      CMPASSERT(negate == FALSE);

      double size = getSampleConstValue();
      size = size / 100; // Size specified as percent

      sampleCondition = new (wHeap) RandomSelection((float)size);
    }
  else if(sampleType() == RelSample::CLUSTER)
    {
      *CmpCommon::diags() << DgSqlCode(-7003);
      GenExit();
      return NULL;
    }
  else
    GenAssert(0, "ItmBalance::codeGen: Unknown sampling method!");

  // At this point the expression for the sample condition for this
  // branch of the balance clause has been computed. Now, combine this
  // expression with any balance predicate and child balance branchs
  // to get an expression representing the entire subtree for this node.
  //
  // PseudoCode:
  //  if(predicate) then sampleCondition else nextBranch;
  //

  // Hack until compiler is changed... set the predicate to null if child(0)
  // is simply ITM_TRUE, otherwise use the predicate.
  //
  ItemExpr *nextBranch = getNextBalance();
  ItemExpr *predicate = getPredicate();
  if(predicate->getOperatorType() == ITM_RETURN_TRUE) predicate = NULL;

  // There cannot be a child branch without a predicate.
  //
  GenAssert(predicate || !nextBranch,
	    "Sampling: Balance: Predicate and nextBranch mismatch!");

  // If there is no predicate the result expression is simply the
  // sampling condition. Otherwise, the result is the sampling condition
  // only if the predicate is true and is the child balance branch if
  // the predicate is false.
  //
  ItemExpr *balanceExpr = sampleCondition;
  if(predicate)
    {
      balanceExpr = new(wHeap)
	Case(NULL,
	     new(wHeap) IfThenElse(predicate, sampleCondition, nextBranch));
    }
  GenAssert(balanceExpr, "balanceExpr failed compilation!");

  // Synthesize the types and value Ids for the new items.
  //
  balanceExpr->synthTypeAndValueId(TRUE);

  // Repalce the orginal value ID with the new item expression.
  //
  getValueId().replaceItemExpr(balanceExpr);

  balanceExpr = balanceExpr->preCodeGen(generator);

  // Clear the nested FIRSTN flag in case it was set.
  //
  generator->setInNestedFIRSTNExpr(FALSE);

  // return the preCodeGen of the new expression.
  //
  return balanceExpr;
}

// ItmBalance::codeGen
//
// ItmBalance should have been transformed away in preCodeGen -- see above.
//
short ItmBalance::codeGen(Generator * /*generator*/)
{
  GenAssert(0, "ItmBalance::codeGen -- Should never get here!");
  return 0;
}

// NotCovered::codeGen NotCovered is codegenned to be a convert clause
// (copy).  Basically, it is a NOOP, but the convert is added here to
// make things simplier.  The extra move should be optimized away
// during PCode generation. Alternatively, the map table could have
// been modified to map the result of this operation to be the same as
// the result of child(0).
//
short NotCovered::codeGen(Generator *generator)
{


  Attributes** attr;

  if(generator->getExpGenerator()->genItemExpr(this, 
                                               &attr,
                                               1 + getArity(),
                                               -1) == 1)
    return 0;
  
  ex_conv_clause * conv_clause =
    new(generator->getSpace()) ex_conv_clause(ITM_CAST,
                                              attr,
                                              generator->getSpace());
  generator->getExpGenerator()->linkClause(this, conv_clause); 

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// class RandomSelection
////////////////////////////////////////////////////////////////////////////
short RandomSelection::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, 
                                                &attr, 
                                                (1 + getArity()), 
                                                -1
                                                ) == 1)
    return 0;
  
  ex_clause *function_clause = new (space) 
                 ExFunctionRandomSelection(ITM_RAND_SELECTION,
                                           attr, 
                                           space,
                                           getSelProbability()
                                          );

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}
