/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         RelStoredProc.h
 * Description:  RelExpr classes for stored procedure.
 *               
 *               
 * Created:      03/12/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef RELSTOREDPROC_H
#define RELSTOREDPROC_H

#include "ObjectNames.h"
#include "RelMisc.h" 
#include "RelRoutine.h" 

// consider moving all of this into RelRoutine.cpp ..


class RelInternalSP : public TableValuedFunction
{
public:
  // constructor
  RelInternalSP(const NAString & name ="", ItemExpr* params = 0,
		OperatorTypeEnum otype = REL_INTERNALSP,
                CollHeap* oHeap=0, UInt16 arkcmpInfo = 0);

  // copy ctor
  RelInternalSP(const RelInternalSP & other, CollHeap * h=0) ;
  
  virtual ~RelInternalSP();

  // enum for arkcmpInfo_ flag

  enum
  {
    executeInSameArkcmp     = 0x1,
    requiresTMFTransaction  = 0x2,
    suppressDefaultSchema   = 0x4,
    executeInLocalProcess = 0x8
  };

  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);
  
  // method to do code generation
  virtual short codeGen(Generator*);

  virtual void getPotentialOutputValues(ValueIdSet &vs) const;
  
  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;
  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);
  virtual CostMethod* costMethod() const;


  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  virtual void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet & newExternalInputs,
                                   ValueIdSet& predOnOperator,
				   const ValueIdSet * nonPredExprOnOperator = NULL,
                                   Lng32 childId = (-MAX_REL_ARITY) );

  // method for transformer.
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe); 

  virtual void recomputeOuterReferences();

  // method for normalizer
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual const char *getVirtualTableName()	{ return outTableName_; }

  ValueIdList & procTypes()			{ return procTypes_; }

  NABoolean pilotAnalysis(QueryAnalysis* qa) {return FALSE;}

  // accessors  - get flag bit value
  inline NABoolean getExecuteInSameArkcmp (void) 
    { return (arkcmpInfo_ & executeInSameArkcmp); };
  inline NABoolean getRequiresTMFTransaction (void) 
    { return (arkcmpInfo_ & requiresTMFTransaction); };
  inline NABoolean getSuppressDefaultSchema (void) 
    { return (arkcmpInfo_ & suppressDefaultSchema); };

  NABoolean isQueryCacheVirtualTable() const ;
  NABoolean isHybridQueryCacheVirtualTable() const;
   
private:
  //assignment operator
  const RelInternalSP& operator=(const RelInternalSP&);
  // the stored procedure name
  NAString procName_;
  // the output table name, used to index NATable
  NAString outTableName_;
  // the input parameter types, a list of NATypes, i.e. NATypeToItem
  ItemExpr* procTypesTree_; 
  ValueIdList procTypes_;

  // Bitmap that contains arkcmp-related information passed from the compiler
  // to the executor. Internal stored procedures use it to indicate that
  // the procedure in execution corresponds to query cache statistics
  UInt16 arkcmpInfo_;
  
}; // class RelInternalSP

class RelUtilInternalSP : public RelInternalSP
{
public:
  // constructor
  RelUtilInternalSP(const NAString & name ="", ItemExpr* params = 0,
		    OperatorTypeEnum otype = REL_UTIL_INTERNALSP,
		    CollHeap* oHeap=0, UInt16 arkcmpInfo = 0) :
       RelInternalSP(name, params, otype, oHeap, arkcmpInfo)
  {
  }

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual void getPotentialOutputValues(ValueIdSet &vs) const;
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

}; // class RelUtilInternalSP


// Standalone function to determine the requirements of built-in functions
// activated through the TDMISP mechanism
UInt16 getTdmispArkcmpInfo (const char * actualSP);

#endif
