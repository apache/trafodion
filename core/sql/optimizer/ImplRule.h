#ifndef IMPLRULE_H
#define IMPLRULE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ImplRule.h
* Description:  Implementation rules
*               
*               
* Created:	9/14/94
* Language:     C++
*
*
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
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Rule.h"

// It is found that DP2 goes into infinite loop if we have rowsize 
// of approximately > 30000. Internally the executor has a max buffer
// size of 31000 for nodes with remotepartition and 56000 for local 
// partitions.30000 bytes limit is experimentally found to be safe after
// taking into consideration the additional bytes that will be added for
// header information and control information in executor.
#define ROWSIZE_TO_EXECUTE_IN_DP2 30000
// increasing this limit for the DB limits project (see .cpp file)
#define ROWSIZE_TO_EXECUTE_IN_DP2_DBL 55000
// -----------------------------------------------------------------------
// classes defined in this file
// -----------------------------------------------------------------------
class HbaseDeleteRule;
class HbaseUpdateRule;
class HiveInsertRule;
class HbaseInsertRule;
class ExchangeEnforcerRule;
class HbaseScanRule;
class FileScanRule;
class GenericUpdateRule;
class HashGroupByRule;
class HashJoinRule;
class MergeJoinRule;
class NestedJoinRule;
class PhysCompoundStmtRule;
class PhysicalExplainRule;
class PhysicalHiveMDRule;
class PhysicalMapValueIdsRule;
class PhysicalPackRule;
class PhysicalSequenceRule;
class PhysicalTransposeRule;
class PhysicalTupleRule;
class PhysicalTupleListRule;
class PhysicalUnPackRowsRule;
class PhysicalTMUDFRule;
class PhysicalFastExtractRule;
class SortEnforcerRule;
class SortGroupByRule;
class AggregateRule;
class PhysShortCutGroupByRule;
class UnionRule;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Function to add transformation rules to the rule set
// -----------------------------------------------------------------------
void CreateImplementationRules(RuleSet*);

class HbaseDeleteRule : public Rule
{
public:
  HbaseDeleteRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}
  
  // copy ctor
  HbaseDeleteRule (const HbaseDeleteRule &) ; // not written

  virtual ~HbaseDeleteRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class HbaseDeleteCursorRule : public HbaseDeleteRule
{
public:
  HbaseDeleteCursorRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) : 
       HbaseDeleteRule(name,pattern,substitute) {}

  // copy ctor
  HbaseDeleteCursorRule (const HbaseDeleteCursorRule &) ; // not written

  virtual ~HbaseDeleteCursorRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class HbaseUpdateRule : public Rule
{
public:
  HbaseUpdateRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}
  
  // copy ctor
  HbaseUpdateRule (const HbaseUpdateRule &) ; // not written

  virtual ~HbaseUpdateRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class HbaseUpdateCursorRule : public HbaseUpdateRule
{
public:
  HbaseUpdateCursorRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) : 
       HbaseUpdateRule(name,pattern,substitute) {}

  // copy ctor
  HbaseUpdateCursorRule (const HbaseUpdateCursorRule &) ; // not written

  virtual ~HbaseUpdateCursorRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class HiveInsertRule : public Rule
{
public:
  HiveInsertRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  HiveInsertRule (const HiveInsertRule &) ; // not written

  virtual ~HiveInsertRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
                              Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};

class HbaseScanRule : public Rule
{
public:
  HbaseScanRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
  Rule(name,pattern,substitute) {}

  // copy ctor
  HbaseScanRule (const HbaseScanRule &) ; // not written

  virtual ~HbaseScanRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
                              Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};


class HbaseInsertRule : public Rule
{
public:
  HbaseInsertRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  HbaseInsertRule (const HbaseInsertRule &) ; // not written

  virtual ~HbaseInsertRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
                              Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};

class ExchangeEnforcerRule : public Rule
{
public:
  ExchangeEnforcerRule (const char * name,
                        RelExpr * pattern,
                        RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  ExchangeEnforcerRule (const ExchangeEnforcerRule &) ; // not written

  virtual ~ExchangeEnforcerRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
};

class FileScanRule : public Rule
{
public:
  FileScanRule (const char * name,
                RelExpr * pattern,
                RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  FileScanRule (const FileScanRule &) ; // not written

  virtual ~FileScanRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class HashGroupByRule : public Rule
{
public:
  HashGroupByRule (const char * name,
                   RelExpr * pattern,
                   RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  HashGroupByRule (const HashGroupByRule &) ; // not written

  virtual ~HashGroupByRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};


class HashJoinRule : public Rule
{
public:
  HashJoinRule(const char * name,
               RelExpr * pattern,
               RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  HashJoinRule (const HashJoinRule &) ; // not written

  virtual ~HashJoinRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class MergeJoinRule : public Rule
{
public:
  MergeJoinRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  MergeJoinRule (const MergeJoinRule &) ; // not written

  virtual ~MergeJoinRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canBePruned(RelExpr * expr) const;
};

class NestedJoinRule : public Rule
{
public:
  NestedJoinRule(const char * name,
                 RelExpr * pattern,
                 RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  NestedJoinRule (const NestedJoinRule &) ; // not written

  virtual ~NestedJoinRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canBePruned(RelExpr * expr) const;
};

class NestedJoinFlowRule : public Rule
{
public:
  NestedJoinFlowRule(const char * name,
                     RelExpr * pattern,
                     RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}
  
  // copy ctor
  NestedJoinFlowRule (const NestedJoinFlowRule &) ; // not written

  virtual ~NestedJoinFlowRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysCompoundStmtRule : public Rule
{
public:
  PhysCompoundStmtRule(const char * name,
                       RelExpr * pattern,
                       RelExpr * substitute) :
    Rule(name,pattern,substitute) {}

  // copy ctor
  PhysCompoundStmtRule (const PhysCompoundStmtRule &) ; // not written

  virtual ~PhysCompoundStmtRule() {}
  virtual NABoolean topMatch(RelExpr * expr,
                             Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
}; 

class PhysicalMapValueIdsRule : public Rule
{
public:
  PhysicalMapValueIdsRule(const char * name,
                          RelExpr * pattern,
                          RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalMapValueIdsRule (const PhysicalMapValueIdsRule &) ; // not written

  virtual ~PhysicalMapValueIdsRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;

  // This implementation rule is not context sensitive
  virtual NABoolean isContextSensitive() const;
};

class PhysicalRelRootRule : public Rule
{
public:
  PhysicalRelRootRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalRelRootRule (const PhysicalRelRootRule &) ; // not written

  virtual ~PhysicalRelRootRule();
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysicalSequenceRule : public Rule
{
public:
  PhysicalSequenceRule(const char * name,
                       RelExpr * pattern,
                       RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalSequenceRule (const PhysicalSequenceRule &) ; // not written

  virtual ~PhysicalSequenceRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};

class PhysicalTupleRule : public Rule
{
public:
  PhysicalTupleRule(const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalTupleRule (const PhysicalTupleRule &) ; // not written

  virtual ~PhysicalTupleRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysicalTupleListRule : public Rule
{
public:
  PhysicalTupleListRule(const char * name,
                        RelExpr * pattern,
                        RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalTupleListRule (const PhysicalTupleListRule &) ; // not written

  virtual ~PhysicalTupleListRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};

class PhysicalExplainRule : public Rule
{
public:
  PhysicalExplainRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalExplainRule (const PhysicalExplainRule &) ; // not written

  virtual ~PhysicalExplainRule();
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysicalHiveMDRule : public Rule
{
public:
  PhysicalHiveMDRule(const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalHiveMDRule (const PhysicalHiveMDRule &) ; // not written

  virtual ~PhysicalHiveMDRule();
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysicalPackRule : public Rule
{
public:
  PhysicalPackRule(const char * name,
                   RelExpr * pattern,
                   RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalPackRule (const PhysicalPackRule &) ; // not written

  virtual ~PhysicalPackRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class PhysicalTransposeRule : public Rule
{
public:
  PhysicalTransposeRule(const char * name,
                        RelExpr * pattern,
                        RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalTransposeRule (const PhysicalTransposeRule &) ; // not written

  virtual ~PhysicalTransposeRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};


class PhysicalUnPackRowsRule : public Rule
{
public:
  PhysicalUnPackRowsRule(const char * name,
                         RelExpr * pattern,
                         RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalUnPackRowsRule (const PhysicalUnPackRowsRule &) ; // not written

  virtual ~PhysicalUnPackRowsRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};


class SortEnforcerRule : public Rule
{
public:
  SortEnforcerRule(const char * name,
                   RelExpr * pattern,
                   RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  SortEnforcerRule (const SortEnforcerRule &) ; // not written

  virtual ~SortEnforcerRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
			     Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
};

class SortGroupByRule : public Rule
{
public:
  SortGroupByRule (const char * name,
                   RelExpr * pattern,
                   RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  SortGroupByRule (const SortGroupByRule &) ; // not written

  virtual ~SortGroupByRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class AggregateRule : public SortGroupByRule
{
public:
  AggregateRule (const char * name,
                 RelExpr * pattern,
                 RelExpr * substitute) :
       SortGroupByRule(name,pattern,substitute) {}
  
  // copy ctor
  AggregateRule (const AggregateRule &) ; // not written

  virtual ~AggregateRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context * context);
};

class PhysShortCutGroupByRule : public Rule
{
public:
  PhysShortCutGroupByRule (const char * name,
                           RelExpr * pattern,
                           RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysShortCutGroupByRule (const PhysShortCutGroupByRule &) ; // not written

  virtual ~PhysShortCutGroupByRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class UnionRule : public Rule
{
public:
  UnionRule (const char * name,
             RelExpr * pattern,
             RelExpr * substitute) : Rule(name,pattern,substitute) {}

  // copy ctor
  UnionRule (const UnionRule &) ; // not written

  virtual ~UnionRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};


class PhysicalSampleRule : public Rule
{
public:
  PhysicalSampleRule(const char * name,
                     RelExpr * pattern,
                     RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  PhysicalSampleRule (const PhysicalSampleRule &) ; // not written

  virtual ~PhysicalSampleRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};

class PhysicalInterpretAsRowRule : public Rule
{
   public:
   PhysicalInterpretAsRowRule(const char *name,
                          RelExpr *pattern,
                          RelExpr *substitute) :
        Rule(name,pattern,substitute) {};

   PhysicalInterpretAsRowRule(const PhysicalInterpretAsRowRule &); // not written

   virtual ~PhysicalInterpretAsRowRule();
   virtual NABoolean topMatch(RelExpr *relExpr,
                              Context *context);
   virtual RelExpr * nextSubstitute(RelExpr *before,
                                    Context *context,
                                    RuleSubstituteMemory * & memory);
};

class PhysicalSPProxyFuncRule : public Rule
{
public:

  PhysicalSPProxyFuncRule(const char *name, RelExpr *pattern, RelExpr *substitute)
    : Rule(name,pattern,substitute)
  {}
  
  // copy ctor
  PhysicalSPProxyFuncRule(const PhysicalSPProxyFuncRule &); // not written

  virtual ~PhysicalSPProxyFuncRule();
  virtual NABoolean topMatch(RelExpr *relExpr,
                             Context *context);

  virtual RelExpr *nextSubstitute(RelExpr *before,
                                  Context *context,
                                  RuleSubstituteMemory *&memory);
};

class PhysicalExtractSourceRule : public Rule
{
public:

  PhysicalExtractSourceRule(const char *name, RelExpr *pattern, RelExpr *substitute)
    : Rule(name,pattern,substitute)
  {}
  
  // copy ctor
  PhysicalExtractSourceRule(const PhysicalExtractSourceRule &); // not written

  virtual ~PhysicalExtractSourceRule();
  virtual NABoolean topMatch(RelExpr *relExpr,
                             Context *context);

  virtual RelExpr *nextSubstitute(RelExpr *before,
                                  Context *context,
                                  RuleSubstituteMemory *&memory);
};

class PhysicalIsolatedScalarUDFRule : public Rule
{
public:
  PhysicalIsolatedScalarUDFRule(const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy-ctor, not implemented
  PhysicalIsolatedScalarUDFRule (const PhysicalIsolatedScalarUDFRule &);

  virtual ~PhysicalIsolatedScalarUDFRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PhysicalTMUDFRule : public Rule
{
public:
  PhysicalTMUDFRule(const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy-ctor, not implemented
  PhysicalTMUDFRule (const PhysicalTMUDFRule &);

  virtual ~PhysicalTMUDFRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return TRUE; }
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class PhysicalFastExtractRule : public Rule
{
public:
  PhysicalFastExtractRule(const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy-ctor, not implemented
  PhysicalFastExtractRule (const PhysicalFastExtractRule &);

  virtual ~PhysicalFastExtractRule();
  virtual NABoolean topMatch(RelExpr * relExpr,
                             Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);

   virtual NABoolean isImplementationRule () const { return TRUE; }
};
#endif // IMPLRULE_H
