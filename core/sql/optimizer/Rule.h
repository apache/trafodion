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
#ifndef RULE_H
#define RULE_H
/* -*-C++-*-
******************************************************************************
*
* File:         Rule.h
* Description:  Optimization rule base class
*
*
* Created:	9/14/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "BaseTypes.h"
#include "Collections.h"
#include "SharedPtr.h"

// -----------------------------------------------------------------------
// classes defined in this file
// -----------------------------------------------------------------------
class Rule;
class RuleSubset;
class RuleSet;
class RuleSubstituteMemory;
class Guidance;

struct RuleWithPromise;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------

class RelExpr;
class CutOp;
class Context;
class WildCardOp;
class EstLogProp;
typedef IntrusiveSharedPtr<EstLogProp> EstLogPropSharedPtr;

// -----------------------------------------------------------------------
// global variables
// -----------------------------------------------------------------------
extern THREAD_P NAUnsigned HashJoinRuleNumber;
extern THREAD_P NAUnsigned NestedJoinRuleNumber;
extern THREAD_P NAUnsigned MergeJoinRuleNumber;
extern THREAD_P NAUnsigned JoinToTSJRuleNumber;
extern THREAD_P NAUnsigned JoinCommutativityRuleNumber;
extern THREAD_P NAUnsigned JoinLeftShiftRuleNumber;

// Set this to a reasonable limit for the number of rules and increase
// it as we run out of space.
//
#define MAX_RULE_COUNT 100
#define MAX_NUM_OF_PASSES 2

// -----------------------------------------------------------------------
// global function to reinit the rule set
// -----------------------------------------------------------------------
extern void ReinitRuleSet(RuleSet*);

// -----------------------------------------------------------------------
//     Rules
//     =====
//     There are two kinds of rules, transformation and implementation
//     rules.  Inasmuch as operators can be both logical and physical, a
//     single rule can be both transformation and implementation rule.
//     These kinds of rules are distinguished only by the substitutes they
//     create.
//
//     Rules can be very simple to very complex.  The default rule type is
//     the simplest one.  Simple rules rely entirely on the optimizer to
//     include a copy of the rule's substitute in the "memo" structure.
//     Operator-argument rules rely on the optimizer to create the
//     transformed expression but provide a method that modifies the
//     transformed expression's operators and their arguments (not yet
//     supported).  Function rules provide a method that creates zero or
//     more expressions to be included.  The type of a rule is indicated
//     by its rule_type() method.
//
//     The method topMatch() invokes the RelExpr::match() method to
//     determine whether an expression in "memo" matches a rule's
//     pattern.  Notice that the method associated with the op-arg
//     component in the pattern is called; thus, operators that permit a
//     set of possible values (e.g., commutative binary operator) are
//     permissible.
//
//     For each rule, there are two methods that determine whether or not
//     a rule is applicable given the current optimization guidance and
//     context.  The first method is invoked after an operator matches a
//     rule's top operator.  This method determines a rule's promise.  A
//     rule with non-positive promise is not explored further.  By
//     default, a transformation rule has a promise of 1 and an
//     implementation rule a promise of 2.  Notice that a rule's promise
//     is evaluated before children of the top-most operator are expanded
//     (searched, transformed) and matched against the rule's pattern; the
//     promise value is used to decide whether or not to expand those
//     children.
//
//     A second method determines whether or not a rule actually applies.
//     The second condition method considers an entire binding.  In other
//     words, when this method is invoked, an entire expression tree
//     corresponding to the rule's pattern is available, bound, and passed
//     to the condition function.
//
//     Each rule includes an iterator "nextSubstitute" that produces an
//     RelExpr tree, which the optimizer will treat as an equivalent
//     expression to the one being passed to the method.
//
//     The method "nextSubstitute" is an iterator -- in each invocation,
//     it generates a new substitute.  This iteration ends when the method
//     returns a NULL result.  A NULL result in the first invocation of
//     this method is legal; thus, it is possible to embed any condition
//     in this method.
//
//     Each rule needs to be internally consistent, meaning that:
//     - leaves are numbered 0, 1, 2, ..., (arity-1)
//     - all leaf numbers up to (arity-1) are used in the pattern
//     - each leaf number is used exactly once in the pattern
//     - the substitute uses only leaf numbers in the pattern
//     - (each leaf number may appear 0, 1, 2, or more times)
//     - all operators in the pattern are logical operators
//     - all operators except the root in the substitute are logical
//     - wildcard designators in the pattern are distinct
//     - wildcard designators in the substitute refer to existing wilcards
//       in the pattern
//     - each node in the pattern or substitute has as many children as
//       its arity (leaf and tree nodes have arity 0), this includes wildcard
//       operators which must have a fixed arity
//
//     After a rule has been applied during optimization
//     or exploration, a new guidance structure must be created
//     for optimizing and exploring the newly created expression
//     and its children.
// -----------------------------------------------------------------------

class Rule : public NABasicObject
{

friend class RuleSet;

public:

  Rule (const char * name,
	RelExpr * pattern,
	RelExpr * substitute);

  // copy ctor
  Rule (const Rule &) ; // not written

  virtual ~Rule ();
  virtual void print (FILE * f = stdout,
		      const char * prefix = "",
		      const char * suffix = "");

  // methods to get instance variables / data members
// method is used by debug code and therefore not exercised in mainline code
  inline const char * getName () const                   { return name_; }
  inline RelExpr * getPattern () const                { return pattern_; }
  inline NAUnsigned getNumber() const              { return ruleNumber_; }
  inline RelExpr * getSubstitute () const          { return substitute_; }

  // ---------------------------------------------------------------------
  // Specify some properties of the rule that help scheduling it with
  // more performance. Note that a rule can return any combination of
  // TRUE/FALSE values for the methods below.
  //
  // - a rule is an implementation rule, if it (sometimes) returns a
  //   physical node as the top node of its substitute
  // - a rule is a transformation rule, if it (sometimes) returns a
  //   logical node as the top node of its substitute
  // - a rule is an enforcer rule, if it enforces a physical property
  //   upon expressions in a group
  // - a rule is context sensitive, if it uses the current optimization
  //   context in any of its methods
  // - a rule is pass sensitive if it checks the current optimization
  //   pass in any of its methods
  // - a rule is pattern sensitive if it checks the explore patten in
  //   any of its methods
  // ---------------------------------------------------------------------
  virtual NABoolean isImplementationRule() const;
  virtual NABoolean isTransformationRule() const;
  virtual NABoolean isEnforcerRule() const;
  virtual NABoolean isContextSensitive() const;
  virtual NABoolean isPassSensitive() const;
  virtual NABoolean isPatternSensitive() const;
  virtual NABoolean canBePruned(RelExpr * expr) const;

  // a quick check - without materializing a binding -
  // whether the rule can fire
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);

  // get the result(s) of firing the rule
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);

  // for transformation rules:
  // guidance for exploring and optimizing substitute

  virtual Guidance * guidanceForExploringSubstitute(Guidance * guidance);

  virtual Guidance * guidanceForOptimizingSubstitute(Guidance * guidance,
						     Context * context);

  // for implementation rules:
  // guidance for exploring and optimizing children

  virtual Guidance * guidanceForExploringChild(Guidance * guidance,
					       Context * context,
					       Lng32 childIndex);

  virtual Guidance * guidanceForOptimizingChild(Guidance * guidance,
						Context * context,
						Lng32 childIndex);

  // only for transformation rules, ie. if top op-arg is logical
  // default value is 10000, resulting in exhaustive search
  virtual Int32 promiseForExploration(RelExpr * relExpr,
				      RelExpr * pattern,
				      Guidance * guidance);

  // for all rules; determine promise for applying this rule on relExpr
  // default value is 10000, resulting in exhaustive search
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);

  // A quick check to determine whether the firing of this rule can
  // potentially generate an expression which matches the specified mustMatch
  // pattern.
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;

private:

  const char	      * name_;	// for tracing and debugging
  RelExpr	* pattern_;	// pattern to match
  RelExpr	* substitute_;	// replacement for pattern
  NAUnsigned      ruleNumber_;  // for easy access

  void prepare();	// check rule and prepare it for use

  // gather information about leaf nodes and bind matching leaf nodes together
  static RelExpr *checkAndBindPattern(RelExpr * patt,
				      ARRAY(CutOp *) & leaves,
				      ARRAY(WildCardOp *) & wildcards,
				      NABoolean isSubstitute = FALSE);

}; // Rule

// -----------------------------------------------------------------------
// A rule subset identifies a subset of the global "rule_set" to be applied
// in a certain phase or with a certain guidance
// -----------------------------------------------------------------------

class RuleSubset : public SUBARRAY(Rule *)
{
public:

  RuleSubset(CollHeap* h);

  RuleSubset(ARRAY(Rule *) *superset,CollHeap* h) :
    SUBARRAY(Rule *)(superset,h) {}

  // copy ctor
  RuleSubset (const RuleSubset & orig, CollHeap * h=0) ; // not written

  // warning elimination (removed "inline")
  virtual ~RuleSubset() {}

  inline RuleSubset & operator = (const RuleSubset &other)
    { SUBARRAY(Rule *)::operator = (other); return *this; }
};

// -----------------------------------------------------------------------
// The set of rules used in Cascades
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
class RuleSet : public NABasicObject
{
friend class RuleSubset;

public:

  // constructor
  RuleSet(Int32 approxNumRules, CollHeap* h);

  // copy ctor
  RuleSet (const RuleSet &) ; // not written

  // destructor
  ~RuleSet();

  // ---------------------------------------------------------------------
  // Optimization passes
  // ===================
  // Following Rosenthal, Dayal, and Reiner's suggestion and the
  // implementations in the EXODUS and Volcano optimizer generators,
  // optimization proceeds in multiple passes, where each optimization
  // pass typically increases the search space explored.
  // Method setLastPassNumber() sets the total number of optimization
  // passes, sensitive to the OPTIMIZATION_LEVEL desired, as specified
  // in the defaults table.
  // ---------------------------------------------------------------------
  static Lng32 getFirstPassNumber()                           { return 1; }
  static Lng32 getSecondPassNumber()                          { return 2; }

  // Methods for dealing with the total number of passes.
  void setTotalPasses();
  inline Lng32 getTotalNumberOfPasses() const      { return totalPasses_; }

  inline Lng32 getLastPassNumber() const           { return totalPasses_; }

  // Methods dealing with the current pass number.
  inline void initializeCurrentPassNumber()          { currentPass_ = 0; }
// method is called in an old code path not exercised any more
// this gets called when RelExpr::optimize was used as the optimization
// driver. The new driver (for quite some time now) is RelExpr::optimize2
  inline void incrementCurrentPassNumber()             { currentPass_++; }

  inline Lng32 getCurrentPassNumber() const        { return currentPass_; }

  // Check whether the optimizer is in its last pass.
  inline NABoolean inLastPass() const
               { return (getCurrentPassNumber() == getLastPassNumber()); }

  // done before optimizing a query, reset rule set to initial state
  void initializeFirstPass();

  // second phase of initialization, can't be done in constructor
  void initializeAllPasses();

  // switch to the next pass
  NABoolean nextPass();

  void setCurrentPassNumber(Lng32 passNumber);

  // return a (sub) set of currently applicable rules
  // (see also Guidance::applicableRules())
// warning elimination (remove "inline")
  const RuleSubset * applicableRules()
   {
     CMPASSERT( (currentPass_ >= getFirstPassNumber()) AND
             (currentPass_ <= getLastPassNumber()) );
     return passNRules_[currentPass_];
   }

  // used to recognize first pass rules for the use
  // in optimization Level 1
// warning elimination (removed "inline")
  const RuleSubset * getPassNRules(Lng32 passNum)
   {
     CMPASSERT( (passNum >= getFirstPassNumber()) AND
             (passNum <= getLastPassNumber()) );
     return passNRules_[passNum];
   }

  // get a rule
  Rule * rule(NAUnsigned ruleNo) { return allRules_[ruleNo]; }

  // insert a new rule into the rule set
  void insert(Rule * r);

  // enable a rule for a certain optimization pass starting from
  // fromPass upto and including toPassIncl.
  void enable(NAUnsigned ruleNo,
	      Lng32 fromPass,
	      Lng32 toPassIncl);

  void enable(NAUnsigned ruleNo, Lng32 fromPass)
                         { enable(ruleNo, fromPass, MAX_NUM_OF_PASSES); }

  void enable(NAUnsigned ruleNo)
             { enable(ruleNo, getFirstPassNumber(), MAX_NUM_OF_PASSES); }

  // disable a rule for a certain optimization pass starting from
  // fromPass upto and including toPassIncl.
  void disable(NAUnsigned ruleNo,
	      Lng32 fromPass,
	      Lng32 toPassIncl);

  void disable(NAUnsigned ruleNo, Lng32 fromPass)
                         { disable(ruleNo, fromPass, MAX_NUM_OF_PASSES); }

  void disable(NAUnsigned ruleNo)
             { disable(ruleNo, getFirstPassNumber(), MAX_NUM_OF_PASSES); }

  // accessor functions
  inline Lng32 getCountOfRules() const       { return allRules_.entries(); }
// method is used in debugging and therefore not exercised in mainline code
  inline Int32 getRuleApplCount() const            { return ruleApplCount_; }
  inline void bumpRuleApplCount()                     { ruleApplCount_++; }

  inline const RuleSubset & oldRules()                { return oldRules_; }
  inline const RuleSubset & transformationRules()   { return transRules_; }
  inline const RuleSubset & implementationRules()    { return implRules_; }
  inline const RuleSubset & enforcerRules()      { return enforcerRules_; }
  inline const RuleSubset & contextSensitiveRules()
                                         { return contextSensitiveRules_; }
  inline const RuleSubset & passSensitiveRules()
                                            { return passSensitiveRules_; }
  inline const RuleSubset & patternSensitiveRules ()
                                         { return patternSensitiveRules_; }

private:

  ARRAY(Rule *)       allRules_;      // array with all the rules in it
  // the subsets below don't have to be distinct in any way
  ARRAY(RuleSubset *) passNRules_;    // rules for pass n
  RuleSubset          oldRules_;      // rules used in previous passes

  Lng32                currentPass_;   // current pass of optimization
  Lng32                totalPasses_;   // total number of optimization passes
                                      // for this statement
  Int32                 ruleApplCount_; // statistics on rule applications

  RuleSubset          transRules_;    // all transformation rules
  RuleSubset          implRules_;     // all implementation rules
  RuleSubset          enforcerRules_; // all enforcer rules
  RuleSubset          contextSensitiveRules_; // all context sensitive rules
  RuleSubset          passSensitiveRules_; // all rules that look at the pass #
  RuleSubset          patternSensitiveRules_; // rules using the expl. pattern
};
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// Rule substitute memory
// ======================
//
// This class can be used by rules that generate multiple
// substitutes. The class contains all information that is needed
// to generate a sequence of substitutes and to signal back to the
// invoker of Rule::nextSubstitute whether more substitutes are available.
// This class implements a simple solution for a rule substitute memory:
// produce all substitutes in the first call to Rule::nextSubstitute and
// then just return the rest in subsequent calls
// -----------------------------------------------------------------------

class RuleSubstituteMemory : public LIST(RelExpr *)
{

public:

  RuleSubstituteMemory(CollHeap* h) : LIST(RelExpr *)(h)
    {}

  // copy ctor
  RuleSubstituteMemory (const RuleSubstituteMemory &) ; // not written

  virtual ~RuleSubstituteMemory();

  RelExpr * getNextSubstitute();
};

// -----------------------------------------------------------------------
// an applicable rule that has a certain promise value
// -----------------------------------------------------------------------
struct RuleWithPromise
{
  Int32   promise;
  Int32   tieBreaker;// used to break ties if promise is equal
  Rule    * rule;
};

// -----------------------------------------------------------------------
// This class associates a set of rules with a context.  This mapping
// shows which rules have been applied for a given context.
// -----------------------------------------------------------------------
class RulesPerContext : public NABasicObject
{
public:
  RulesPerContext (const Context* const context,CollHeap* h);

  // copy ctor
  RulesPerContext (const RulesPerContext &) ; // not written

// destructors a generally not called for objects involved in
// optimization (unless the class is heavy weight). This is 
// because the entire statement heap is discarded at the end
// of compilation.
  ~RulesPerContext() {}

  // Accessor methods
  inline const Context* getContext() const { return context_; }
  inline const RuleSubset & getTriedRules() const { return triedRules_; }

  // Manipulation methods
  inline RuleSubset &triedRules() { return triedRules_; }

private:
  const Context* const context_;
  RuleSubset  triedRules_;

};  // RulesPerContext

// -----------------------------------------------------------------------
// This class contains a collection of mappings between a context
// and a set of rules applied on this context.
// -----------------------------------------------------------------------
class RulesPerContextList : public LIST (RulesPerContext *)
{
public:

  RulesPerContextList(CollHeap* h) : LIST(RulesPerContext *)(h)
    {}

  // copy ctor
  //RulesPerContextList (const RulesPerContextList &) ; // not written

  // Has this rule been applied for a given context?
  NABoolean applied (const Context* const context,
		     NAUnsigned ruleNumber) const;

  // Get the rules that were tried for this context
  const RuleSubset & getTriedRules(const Context* const context) const;

  // Has this rule been applied in a previous context with the specified
  // set of input logical properties?
  NABoolean applied (const EstLogPropSharedPtr& inputLogProp,
		     NAUnsigned ruleNumber) const;

  // Add the following rule for the specified context
  void addRule (const Context* const context,
		NAUnsigned ruleNumber);

  // Remove the following rule for the specified context
  void removeRule (const Context* const context,
		NAUnsigned ruleNumber);

private:

};


/*
// -----------------------------------------------------------------------
//     Search guidance
//     ===============
//     Search guidance is created by a rule.  Guidance is distinguished
//     from properties as it applies to the search, not to the
//     intermediate results of the operator trees being manipulated.  A
		     NAUnsigned ruleNumber) const;

  // Add the following rule for the specified context
  void addRule (const Context* const context, 
		NAUnsigned ruleNumber);

  // Remove the following rule for the specified context
  void removeRule (const Context* const context, 
		NAUnsigned ruleNumber);

private:
  
};
*/



// -----------------------------------------------------------------------
//     Search guidance
//     ===============
//     Search guidance is created by a rule.  Guidance is distinguished
//     from properties as it applies to the search, not to the
//     intermediate results of the operator trees being manipulated.  A
//     typical use for "guidance" is to apply a rule only once, e.g., a
//     commutativity rule.  A more sophisticated use may divide the search
//     activity into distinct activities (e.g., groups of rules), and
//     guidance is used to ensure that one activity is performed before
//     another.  In particular, the "guidance" is taken into consideration
//     in rules' promise and condition functions.
// -----------------------------------------------------------------------

class Guidance : public ReferenceCounter
{
public:

  // I guess we just use the default, built-in ctors for this class ...
  // (is this really a good idea?)

  virtual ~Guidance();

  // return a (sub) set of currently applicable rules
  virtual const RuleSubset * applicableRules();

};  // Guidance

// -----------------------------------------------------------------------
// Promise values (more subtle differences may be defined in files
// ImplRule.h and TransRule.h).
//
// Rules that should be unconditionally applied (no further consideration
// of the before-image) have the highest promise. Implementation rules
// have a higher promise than transformation rules, since they lead to
// actual plans that provide cost limits for further rule applications.
// For now, enforcer rules have a lower promise than all other rules.
// Note that individual rules always may
//
// -----------------------------------------------------------------------

// To get rid of annoying warnings, MSVC requires the following (int) casts:
//
const Int32 AlwaysBetterPromise        = (Int32) 99000;
const Int32 DefaultImplRulePromise     = (Int32) 20000;
const Int32 DefaultTransRulePromise    = (Int32) 10000;
const Int32 DefaultEnforcerRulePromise = (Int32) 5000;
const Int32 NoPromise                  = (Int32) 0;

#endif // RULE_H
