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
#ifndef REL3GL_H
#define REL3GL_H

/* -*-C++-*-
******************************************************************************
*
* File:         Rel3GL.h
* Description:  PSM/3GL operators.
*
* Created:      11/05/97
* Language:     C++
*
*
******************************************************************************
*/
#include "RelExpr.h"
#include "Rule.h"
#include "RelMisc.h"
#include "RelSet.h"

//////////////////////////////////////////////////////////////////////////
//
// Contents.
//
//////////////////////////////////////////////////////////////////////////
class CompoundStmt;
class Loop;

//////////////////////////////////////////////////////////////////////////
//
// Forward References.
//
//////////////////////////////////////////////////////////////////////////
class BindWA;
class NormWA;

//////////////////////////////////////////////////////////////////////////
//
// CompoundStmt.
//
//////////////////////////////////////////////////////////////////////////
class CompoundStmt : public RelExpr
{

public:
  // Standared constructor/destructor/accessor/mutators.
  CompoundStmt(RelExpr *leftChild, 
               RelExpr *rightChild,
	       OperatorTypeEnum op = REL_COMPOUND_STMT, 
	       CollHeap *heap = CmpCommon::statementHeap()) :
    RelExpr(op, leftChild, rightChild, heap) { setNonCacheable(); }

  CompoundStmt(const CompoundStmt &other) :
    RelExpr(other.getOperatorType(), other.child(0), other.child(1)) 
    { setNonCacheable(); }

  virtual ~CompoundStmt() {}

  virtual Int32 getArity() const { return 2; }

  // Binder methods.
  virtual RelExpr *bindNode(BindWA*); 

  // Transformation methods.
  virtual void transformNode(NormWA&, ExprGroupId&);
  virtual void recomputeOuterReferences();

  // Normalizer methods.
  virtual RelExpr *normalizeNode(NormWA&);
  virtual void pullUpPreds();
  virtual void rewriteNode(NormWA&);
  virtual void pushdownCoveredExpr
			  (const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet & predicatesOnParent,
			   const ValueIdSet * setOfValuesReqdByParent = NULL,
			   Lng32 childIndex = (-MAX_REL_ARITY));

  // Optimizer methods.
  virtual RelExpr *copyTopNode(RelExpr * = NULL, CollHeap* = NULL); 

  // PreCodeGen methods
  virtual RelExpr *preCodeGen(Generator * generator, 
			      const ValueIdSet & externalInputs,
			      ValueIdSet &pulledNewInputs);

  // GUI methods.
  virtual const NAString getText() const { return "CompoundStmt"; }

  Context* createContextForAChild(Context* myContext,
			  	                  PlanWorkSpace* pws,
				                  Lng32& childIndex);

protected:

  // Internal support methods.
  void enterVEGRegion(NormWA&, Int32 id, NABoolean create=FALSE);
  void leaveVEGRegion(NormWA&, Int32 id);

}; 

class PhysCompoundStmt : public CompoundStmt
{
public:
  // Standard constructor/destructor/accessor/mutators.
  PhysCompoundStmt(RelExpr *leftChild,
                   RelExpr *rightChild,
                   OperatorTypeEnum op = REL_COMPOUND_STMT,
                   CollHeap *heap = CmpCommon::statementHeap()) :
    CompoundStmt(leftChild, rightChild, op, heap ) {}

  PhysCompoundStmt(const CompoundStmt &other) :
    CompoundStmt(other) {}

  virtual ~PhysCompoundStmt() {}

  virtual NABoolean isPhysical() const { return TRUE; }
  virtual NABoolean isLogical() const { return FALSE; }

  virtual CostMethod *costMethod() const;
  // Optimizer methods.
  virtual RelExpr *copyTopNode(RelExpr * = NULL, CollHeap * = NULL);

  virtual  PhysicalProperty *synthPhysicalProperty(const Context*,
                                                   const Lng32,
                                                   PlanWorkSpace *pws); 

  // Code generation methods.
  virtual short codeGen(Generator *);
};


// --------------------------------------------------------------------
// This class is used by assignment statements in compound statements.
// A list of this type is kept in class AssignmentStArea (below) so that 
// it is globally accessible at binding time. This class mantains a current and previous
// valueId, so that when we find SET <var> = <select statement>, we update
// the value id of <var> to that returned by the select statement. See 
// assignment statements internal spec for details. 
// --------------------------------------------------------------------

class AssignmentStHostVars: public NABasicObject
{
public:
  // Constructor
  AssignmentStHostVars(BindWA *bindWA, CollHeap * h = 0) 
    :  bindWA_(bindWA),
       var_(NULL),
       next_(NULL)
      {}

  // Copy constructor
  AssignmentStHostVars(const AssignmentStHostVars &other, CollHeap * h = 0) 
    : bindWA_(other.bindWA_),
      next_(other.next_),
      var_(other.var_)
      {}

  AssignmentStHostVars()  
    { AssignmentStHostVars(NULL); }

  ~AssignmentStHostVars() {}

  // Adds var to end of this list with new value id. The id value overwrites
  // the current value id unless we are inside an IF statement and this
  // is the first time we call this function, in which case the value id is
  // appended, since we will need the old value id when we exit the IF statement
  AssignmentStHostVars & addVar(HostVar *var, const ValueId &id);

  // Adds var to this list with new value id; if it is already there, it
  // overwrites the value id
  AssignmentStHostVars & addToListInIF(HostVar *var, const ValueId &id);

  // Adds all the hostvars and their associated value ids from the AssignmentStHostVars
  // that is passed as an argument to this method to the AssignmentStHostVars pointed to
  // this pointer. The implementation is by repeated calls to addToListInIF() .
  void addAllToListInIF(AssignmentStHostVars * copyFromList) ;

  // Gets current value id 
  const ValueId currentValueId();

  // Update the current value id of this host var
  void setCurrentValueId(const ValueId &id);

  // Gets the variable
  HostVar *& var();

  // Finds the variable whose name is given
  AssignmentStHostVars * findVar(const NAString & name);

  // Finds the variable whose valueId is given
  AssignmentStHostVars * containsValueId(ValueId valueId);

  // Returns next element
  inline AssignmentStHostVars * next() { return next_; }
                         
  // When we reach a Root node that contains a list of host variables on the
  // left hand side of an assignment statement, we update the value ids
  // of such variables with the value ids returned from the subtree below
  // the Root node. This function is called in RelRoot::bindNode
  NABoolean updateValueIds(const ValueIdList &returnedList, ItemExpr *listInRootNode);
  
  // Remove the current value id of this variable
  void removeLastValueId()
  {
    valueIds_.removeAt(valueIds_.entries() - 1);
  }

  // To acces list of value ids
  ValueIdList &valueIds() { return valueIds_; }

  // To know if there are rowsets in the given list
  NABoolean containsRowsets(ItemExpr * list);

  // To perform necessary transformations in case we have rowsets in the SET statement
  // on the left side. We must replace the Root node where the rowset variables appear
  // with a RowsetInto node
 RelExpr * processRowsets(ItemExpr * list, RelExpr * thisNode);

 // Given a list of Host variables with value ids, we enter them in our list
 void enterHostVars(ItemExpr *listOfVars);

private:

   // The variable
   HostVar *var_;

   // A list of the value ids this variable gets. The last one in the list
   // is the current one
   ValueIdList valueIds_;

   // To have access to the rest of the structure
   BindWA *bindWA_;

   // Next element
   AssignmentStHostVars *next_;

};


class Union;

// --------------------------------------------------------------------
// This class is used by assignment statements in compound statements.
// A list of this type is kept in class BindWA so that it is globally
// accessible at binding time. This class mantains data needed by the
// aforementioned project. See assignment statements internal spec for 
// details. 
// --------------------------------------------------------------------

class AssignmentStArea : public NABasicObject
{
public:

  // Constructor
  AssignmentStArea(BindWA *bindWA, CollHeap * h = 0) 
    : listOfVars_(NULL),
      currentIF_(NULL),
      bindWA_(bindWA)
      {}

  // Copy constructor
  AssignmentStArea(const AssignmentStArea &other, CollHeap * h = 0) 
    : listOfVars_(other.listOfVars_),
      currentIF_(other.currentIF_),
      bindWA_(other.bindWA_)
      {}

  AssignmentStArea()  
    { AssignmentStArea(NULL); }

  ~AssignmentStArea() {}

  // Please see comments for currentIF_ below.
  Union * getCurrentIF() { return currentIF_; }
  void setCurrentIF(Union * node) { currentIF_ = node; }

  // Gets pointer to list of variables
  AssignmentStHostVars *&getAssignmentStHostVars() { return listOfVars_; }

  // Updates listOfVars_ when we exit a Union node at binding time. For
  // each host variable, it determines whether its value id is no longer
  // valid outside the IF statement we are exiting, and removes it if so.
  // In this way, the value id this variable had before entering the IF 
  // statement will be the current one
  void updateValueIdsTable(Union * node);

  // Removes the last value ids assigned to the given node during 
  // binding of childNumber
  void removeLastValueIds(AssignmentStHostVars * listInUnion, Union * node);

private:

   // List of host variables in the statement
   AssignmentStHostVars * listOfVars_;

   // To know which is the Union node that is the immediate ancestor of
   // the node we are currently in. This gets used when we have SET statements
   // within an IF statement. 
   Union *currentIF_;

   BindWA * bindWA_;

};

#endif /* REL3GL_H */

