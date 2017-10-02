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
* File:         Rule.C
* Description:  Cascades optimization rules
*
* Created:      9/14/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "TransRule.h"
#include "ImplRule.h"
#include "opt.h"
#include "EstLogProp.h"
#include "DefaultConstants.h"
#include "CmpContext.h"


// -----------------------------------------------------------------------
// Reinitialize the rule set *in place*:  to be used only if an exception is
// caught in opt.cpp -- the exception (assertion) needs to unbind all rules
// (context heap) from the memo (statement heap, about to be deleted).
// -----------------------------------------------------------------------
void ReinitRuleSet(RuleSet* rules)
{
  if (rules)
    {
      Lng32 ruleCnt = rules->getCountOfRules();
      while (ruleCnt--)
        {
	  Rule *rule = rules->rule(ruleCnt);
      if (rule->getSubstitute())
	    rule->getSubstitute()->releaseBindingTree(TRUE/*moribundMemo*/);
	}
    }
}

/* ============================================================ */

/*
  Rules
  =====
  both transformation and implementation rules -- By default, a rule
  is a simple rule, meaning that the substitute given in the rule
  sufficiently represents the transformed expression after cut
  operators in the rule have been bound to proper children.
  */

Rule::Rule (const char * name, RelExpr * pattern, RelExpr * substitute)
{
  if (name == NULL)
// all current rules have name, which is a good practice
    name_ = "no user name";
  else
    name_ = name;

  pattern_ = pattern;
  substitute_ = substitute;

  prepare();

  // a pattern can't have a tree node as its top node
  if (pattern_)
    CMPASSERT(NOT pattern_->isSubtreeOp());

} // Rule::Rule

// Rules are created once and never deleted
Rule::~Rule ()
{
  delete pattern_;
  delete substitute_;
} // Rule::~Rule

// print methods are for debugging
void Rule::print (FILE * f, const char * prefix, const char * suffix)
{
  fprintf (f, "%sRule \"%s\" :\n",
	   prefix, name_);
  pattern_ -> print (f, "  Pattern    = ", "");
  substitute_ -> print (f, "  Substitute = ", "");
  fprintf (f, "%s\n", suffix);
} // Rule::print

NABoolean Rule::isImplementationRule() const
{
  // by default, assume that a copy of the substitute is returned
  return (substitute_->isPhysical());
}

NABoolean Rule::isTransformationRule() const
{
  // by default, assume that a rule is not both transformation and
  // implementation rule
  return NOT isImplementationRule();
}

NABoolean Rule::isEnforcerRule() const
{
  // For enforcer rules, the pattern is always a cutOp meaning that
  // it is always applicable.  Enforcer rules are useful for generating
  // physical operators which enforces desired physical properties.
  return (pattern_ && pattern_->isCutOp());
}

NABoolean Rule::isContextSensitive() const
{
  // By default, consider implementation rules (which include
  // enforcer rules) to be context-sensitive.
  return isImplementationRule();
}

NABoolean Rule::isPassSensitive() const
{
  // By default, assume the rule is not pass sensitive
  return FALSE;
}

NABoolean Rule::isPatternSensitive() const
{
  // By default, assume the rule is not pattern sensitive
  return FALSE;
}

NABoolean Rule::canBePruned(RelExpr * expr) const
{
  // By default, any rule can be pruned unless otherwise defined
  // in its own implementation of the function.
  // This function is only called after a successfull topMatch()
  // i.e. within the processing of an ApplyRuleTask.
  // The purpose of this function is to protect some rule-expr
  // pairs from heuristic or randem pruning.
  return TRUE;
}

NABoolean Rule::topMatch(RelExpr * relExpr,
			 Context *)
{
  // default implementation should not be invoked for expression-
  // insensitive rules (like enforcer rules).
  CMPASSERT(relExpr != NULL);

  return relExpr->patternMatch(*pattern_);
}


RelExpr * Rule::nextSubstitute (RelExpr * before,
				Context *,
				RuleSubstituteMemory *& /*memory*/)
{
  RelExpr *result;

  // the default implementation of a rule fires only once, therefore
  // no RuleSubstituteMemory is allocated

  // build the result expression from the rule's pattern, the skeleton
  // of the rule result, and the original expression "before".
  result = substitute_->copyTree(CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  // now set the isinBlockStmt flag for all nodes in the tree.
  result->setBlockStmtRecursively(before->isinBlockStmt());

  return result;
} // Rule::nextSubstitute

Guidance * Rule::guidanceForExploringSubstitute (Guidance *)
{
  return ( NULL );
} // Rule::guidanceForExploringSubstitute

Guidance * Rule::guidanceForOptimizingSubstitute (Guidance *,
			                          Context *)
{
  return ( NULL );
} // Rule::guidanceForOptimizingSubstitute

Guidance * Rule::guidanceForExploringChild(Guidance *,
					   Context *,
					   Lng32)
{
  return ( NULL );
} // Rule::guidanceForExploringChild

Guidance * Rule::guidanceForOptimizingChild(Guidance *,
					    Context *,
					    Lng32)
{
  return ( NULL );
} // Rule::guidanceForOptimizingChild

Int32 Rule::promiseForExploration (RelExpr * expr,
				   RelExpr *,
				   Guidance *)
{
  // by default, explore in exhaustive search
  // (only transformation rules are used for exploration)
  /*if (expr && CURRSTMT_OPTDEFAULTS->pruneByOptDefaults(this, expr))
    return 0;*/
  return ( DefaultTransRulePromise );
} // Rule::promiseForExploration

Int32 Rule::promiseForOptimization (RelExpr * expr,
				    Guidance *,
				    Context *)
{
  /*if (expr && CURRSTMT_OPTDEFAULTS->pruneByOptDefaults(this, expr))
    return 0;*/

  if (isImplementationRule())
    {
      return DefaultImplRulePromise;
    }
  else
    {
      return DefaultTransRulePromise;
    }
} // Rule::promiseForOptimization

void Rule::prepare()
{
  // an array to hold pointers for cuts of the pattern, ordered by index
  // (most rules should get by with 4 cuts and 4 wildcards)
  ARRAY(CutOp *) cuts(HEAP, 4);
  ARRAY(WildCardOp *) wildcards(HEAP, 4);

  // fill the "cuts" array
  if (pattern_)
    (void *) checkAndBindPattern(pattern_,cuts,wildcards,FALSE);

  // now replace all cut nodes in the substitute by the corresponding
  // cut nodes of the pattern
  if (substitute_)
    substitute_ = checkAndBindPattern(substitute_,cuts,wildcards,TRUE);

} // Rule::prepare

RelExpr * Rule::checkAndBindPattern(RelExpr * patt,
				    ARRAY(CutOp *) & cuts,
				    ARRAY(WildCardOp *) & wildcards,
				    NABoolean isSubstitute)
{
  RelExpr *result = patt;

  if (patt->isCutOp())
    {
      CutOp *acut = (CutOp *) patt;
      Int32 ix = acut->getIndex();

      if (!isSubstitute)
	{
	  if (cuts.used(ix))
	    // error, a cut index occurs twice in the pattern
	    ABORT("encountered pattern with duplicate cut nodes");
	  else
	    cuts.insertAt(ix,acut);
	}
      else
	{
	  // this is a substitute, replace the cut node with the
	  // corresponding cut node from the pattern (both pattern and
	  // substitute then point to the same cut node)
	  if (NOT cuts.used(ix))
	    ABORT("cut in substitute doesn't match the pattern");

	  result = cuts[ix];
	  delete patt;
	  patt = result;
	}
    } // pattern is a cut
  else
    if (patt->isWildcard())
      {
	WildCardOp *wild = (WildCardOp *) patt;
	Int32 designator = wild->getDesignator();

	if (!isSubstitute)
	  {
	    // fill in the appropriate entry of the wildcards array

	    if (wildcards.used(designator))
	      // error, a wildcard index occurs twice in the pattern
	      ABORT("pattern with duplicate wildcard designators");
	    else
	      wildcards.insertAt(designator,wild);
	  }
	else
	  {
	    if (NOT wildcards.used(designator))
	      ABORT("wildcard in substitute has an invalid designator");

	    // this is a substitute, associate the wildcard node in the
	    // substitute with the wildcard node in the pattern
	    wild->setCorrespondingNode(wildcards[designator]);

	    // unlike cut nodes, wildcards are NOT shared
	    // between the pattern and the substitute
	  }
      } // pattern is a wildcard

  // recurse

  Int32 arity = patt->getArity();

  for (Lng32 i = 0; i < arity; i++)
    {
      result->child(i) = checkAndBindPattern(patt->child(i)->castToRelExpr(),
					     cuts,
					     wildcards,
					     isSubstitute);
    }

  return result;

} // Rule::checkAndBindPattern

// Can this rule potentially generate the specified pattern?
// NOTE: This method must be defined for those rules that specifies
// multiple substitutes, or those rules for which the substitute is
// not a true indication of the substitute expression.
NABoolean Rule::canMatchPattern (const RelExpr * pattern) const

{
  return substitute_->getOperator().match(pattern->getOperator());
}

// -----------------------------------------------------------------------
// methods for class RuleSubset
// -----------------------------------------------------------------------

RuleSubset::RuleSubset(CollHeap* h) :
  SUBARRAY(Rule *)(&GlobalRuleSet->allRules_,h) {}

RuleSubset::RuleSubset (const RuleSubset & orig, CollHeap * h) :
  SUBARRAY(Rule *)(orig,h) {}


// -----------------------------------------------------------------------
// methods for class RuleSet
// -----------------------------------------------------------------------

RuleSet::RuleSet(Int32 approxNumRules, CollHeap* h) :
  allRules_(h, approxNumRules),
  passNRules_(h),
  oldRules_(&allRules_,h),
  transRules_(&allRules_,h),
  implRules_(&allRules_,h),
  enforcerRules_(&allRules_,h),
  contextSensitiveRules_(&allRules_,h),
  passSensitiveRules_(&allRules_,h),
  patternSensitiveRules_(&allRules_,h),
  ruleApplCount_(0)
{
  initializeCurrentPassNumber();
  setTotalPasses();  // set the total number of optimization passes

  initializeAllPasses();
}

// Rules are created once and never deleted
RuleSet::~RuleSet()
{
  for (Lng32 i = 0; i < (Lng32)allRules_.entries(); i++)
    delete allRules_[i];
}

void RuleSet::insert(Rule * r)
{
  Lng32 num = r->ruleNumber_ = allRules_.entries();
  allRules_.insertAt(num,r);

  if (num >= MAX_RULE_COUNT)
    ABORT("Increase max number of rules in Rule.h");

  if (r->isImplementationRule())
    implRules_ += num;

  if (r->isTransformationRule())
    transRules_ += num;

  if (r->isEnforcerRule())
    enforcerRules_ += num;

  if (r->isContextSensitive())
    contextSensitiveRules_ += num;

  if (r->isPassSensitive())
    passSensitiveRules_ += num;

  if (r->isPatternSensitive())
    patternSensitiveRules_ += num;
}

void RuleSet::setTotalPasses()
{
  NAString value(CmpCommon::statementHeap());
  // Only do the first pass if the optimization level is MINIMUM or
  // if the query complexity is greater than that for 32 regular joins
  // (2^32 = INT_MAX), and the join order is not fixed by the user.
  // This is because we assume that performing more than 32 joins will
  // consume too many resources and take too long, even at MEDIUM opt.
  // level.
  // limit increase to 40 way join i.e. (40 * 2 ^ 39)
  if (!CURRENTSTMT &&
      (CmpCommon::getDefault(OPTIMIZATION_LEVEL) == DF_MINIMUM))
  {
    totalPasses_ = 1;
  }
  else if (!CURRENTSTMT)
  {
    totalPasses_ = MAX_NUM_OF_PASSES;
  }
  else if ((CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MINIMUM) OR
      ((CURRSTMT_OPTDEFAULTS->getQueryComplexity() > 3e13) AND
       NOT CURRSTMT_OPTDEFAULTS->joinOrderByUser()))
  {
    totalPasses_ = 1;
  }
  else
  {
    totalPasses_ = MAX_NUM_OF_PASSES;
  }

  //---------------------------------------------------------
  // Optimization Level = 0 (minimum level) is the same as
  //                      optimization pass 1 today.
  // For now, all other optimization levels equate to full
  // optimization level today (i.e. optimization pass 2).
  //---------------------------------------------------------
}

void RuleSet::enable(NAUnsigned ruleNo, Lng32 fromPass, Lng32 toPassIncl)
{
  CMPASSERT(
    fromPass >= getFirstPassNumber() AND toPassIncl >= getFirstPassNumber());

  if (fromPass <= MAX_NUM_OF_PASSES AND toPassIncl <=  MAX_NUM_OF_PASSES)
    for (Lng32 i = fromPass; i <= toPassIncl; i++)
      *(passNRules_[i]) += ruleNo;
}

void RuleSet::disable(NAUnsigned ruleNo, Lng32 fromPass, Lng32 toPassIncl)
{
  CMPASSERT(
    fromPass >= getFirstPassNumber() AND toPassIncl >= getFirstPassNumber());

  if (fromPass <= MAX_NUM_OF_PASSES AND toPassIncl <=  MAX_NUM_OF_PASSES)
    for (Lng32 i = fromPass; i <= toPassIncl; i++)
      *(passNRules_[i]) -= ruleNo;
}

void RuleSet::initializeAllPasses()
{
  Lng32 index;
  for (index = 0; index < getFirstPassNumber(); index++)
    passNRules_.insertAt(index,NULL);
  for (index = getFirstPassNumber(); index <= MAX_NUM_OF_PASSES; index++)
    passNRules_.insertAt(index,
			 new(CmpCommon::contextHeap())
			 RuleSubset(&allRules_, CmpCommon::contextHeap()));
}

void  RuleSet::initializeFirstPass()
{
  ruleApplCount_ = 0;
  initializeCurrentPassNumber();
  oldRules_.clear();

  // In the case that optimization_level has been reset dynamically
  // via the CONTROL QUERY DEFAULT statement, reset the total pass count.
  setTotalPasses();
}

// this is called by the old optimization driver i.e. RelExpr::optimize
// the new optimization driver is method RelExpr::optimize2
NABoolean RuleSet::nextPass()
{
  if (NOT inLastPass())
    {
      incrementCurrentPassNumber();
      oldRules_ += *applicableRules();
      return TRUE;
    }
  else
    return FALSE;
}

void RuleSet::setCurrentPassNumber(Lng32 passNumber)
{
  currentPass_ = passNumber;
  oldRules_ += *applicableRules();
}
// -----------------------------------------------------------------------
// methods for class RuleSubstituteMemory
// -----------------------------------------------------------------------
RuleSubstituteMemory::~RuleSubstituteMemory() {}

RelExpr * RuleSubstituteMemory::getNextSubstitute()
{
  RelExpr * result;

  if (NOT getFirst(result))
    result = NULL;

  return result;
}

// -----------------------------------------------------------------------
// methods for RulesPerContext
// -----------------------------------------------------------------------
RulesPerContext::RulesPerContext (const Context * const context,CollHeap* h)
                : context_(context)
                , triedRules_(h)
{
  triedRules_.clear();  // start with a clear bitmap
}

// -----------------------------------------------------------------------
// methods for RulesPerContextList
// -----------------------------------------------------------------------

// Get the Rules that have been applied to this context
const RuleSubset & RulesPerContextList::getTriedRules(const Context* const context) const
{
  for (Lng32 i= 0; i< (Lng32)entries(); i++)
  {
    const Context* const other = (*this)[i]->getContext();
    if (context == other)
      return (*this)[i]->getTriedRules();
  }
  RuleSubset* emptyRuleSubset =
    new(CmpCommon::statementHeap()) RuleSubset(CmpCommon::statementHeap());
  emptyRuleSubset->clear();
  return *emptyRuleSubset;
}

// Has the provided rule been applied already for the given context?
NABoolean RulesPerContextList::applied (const Context * const context,
					NAUnsigned ruleNumber) const
{
  Lng32 i = 0;
  while (i < (Lng32)entries())
  {
    const Context* const other = (*this)[i]->getContext();
    if (context == other)
      return ((*this)[i]->getTriedRules().contains (ruleNumber));
    else
      i++;
  }
  return FALSE;
}

// not called anywhere in the code
//
// Has the provided rule been applied in any prior context which has the
// provided set of input logical properties?
NABoolean RulesPerContextList::applied (const EstLogPropSharedPtr& inputLogProp,
					NAUnsigned ruleNumber) const
{
  Lng32 i = 0;
  while (i < (Lng32)entries())
  {
    if ((*this)[i]->getContext() != NULL AND
	(*this)[i]->getContext()->getInputLogProp()->compareEstLogProp(inputLogProp) == SAME)
      return ((*this)[i]->getTriedRules().contains (ruleNumber));
    else
      i++;
  }
  return FALSE;
}

void RulesPerContextList::addRule (const Context* const context,
				   NAUnsigned ruleNumber)
{
  Lng32 i = 0;
  while (i < (Lng32)entries())
  {
    if (context == (*this)[i]->getContext())
    {
      // mark this rule as already applied for this context
      (*this)[i]->triedRules() += ruleNumber;
      return;
    }
    else
      i++;
  }

  // context not found?  Create a new entry for this context.

  RulesPerContext *newEntry = new (CmpCommon::statementHeap())
         RulesPerContext (context,(CmpCommon::statementHeap()));
  newEntry->triedRules() += ruleNumber;

  // insert this new structure at the head of the list
  insertAt (0, newEntry);
}

// method is not called anywhere
void RulesPerContextList::removeRule (const Context* const context,
				   NAUnsigned ruleNumber)
{
  Lng32 i = 0;
  while (i < (Lng32)entries())
  { // is this enough? Do we need to check for the SAME context here?
    if (context == (*this)[i]->getContext())
    {
      // remove this rule from already applied for this context
      (*this)[i]->triedRules() -= ruleNumber;
      return;
    }
    else
      i++;
  }
}

// -----------------------------------------------------------------------
// methods for class Guidance
// -----------------------------------------------------------------------

Guidance::~Guidance() {}

const RuleSubset * Guidance::applicableRules()
{
  // default implementation, return all applicable rules
  return GlobalRuleSet->applicableRules();
}

