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
#ifndef REL_DCL_H
#define REL_DCL_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         RelDCL.h
 * Description:  classes to represent DCL statements.
 *
 *
 * Created:      8/5/1996
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Collections.h"
#include "ComTransInfo.h"
#include "ObjectNames.h"
#include "RelExpr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class RelLock;
class RelTransaction;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class LogicalProperty;
class BindWA;
class Generator;

// -----------------------------------------------------------------------
// class RelLock
//
// This class implements the LOCK and UNLOCK table or view statement.
//
// Note: All non-generator methods are defined in file RelLock.C in
//       the optimizer directory.
//       All generator methods are defined in file RelGenLock.C in
//       the generator directory.
// -----------------------------------------------------------------------
class RelLock : public RelExpr
{
public:

  // constructor
  RelLock(const CorrName& name,
	  LockMode lockMode = SHARE_,
	  NABoolean lockIndex = TRUE, 
	  OperatorTypeEnum otype = REL_LOCK,  // REL_UNLOCK for unlock stmt.
	  NABoolean parallelExecution = FALSE);

  // copy ctor
  RelLock (const RelLock & rel, CollHeap * h=0) ; // not written

  ~RelLock();

  LockMode getLockMode() const			{ return lockMode_; }

  // get and set the table name
  inline const CorrName & getTableName() const  { return userTableName_; }
  inline       CorrName & getTableName()        { return userTableName_; }
  inline const CorrName * getPtrToTableName() const
                                                { return &userTableName_; }

  inline const NAString &getLocationName() const {
    return userTableName_.getLocationName();}
  inline void setLocationName(const NAString& locName) { 
    userTableName_.setLocationName (locName); }
  inline NABoolean isView() const {return isView_;};
  inline void setView()           {isView_ = TRUE;}

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  // method to do code generation.
  // Defined in GenRelDCL.cpp in generator directory.
  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // various PC methods
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const{return 0;};
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);
  virtual const NAString getText() const;

private:
  // the user-specified name of the table or view
  CorrName userTableName_;

  // the mode in which to lock the table or view
  LockMode lockMode_;

  // lock all indices on the tables.
  NABoolean lockIndex_;

  //++ MV - fix lock view
  // the user-specified name is a view
  NABoolean isView_;

  // TRUE, if lock/unlock are to run in parallel mode.
  NABoolean parallelExecution_;

  // list of table names that are to be locked/unlocked.
  // If the input userTableName_ is a view, then this list contains
  // all the base table names that were specified in the view
  // definition.
  // Created by binder.
  LIST(CorrName *) baseTableNameList_;

  // recursively traverses the tree pointed to by 'node' which is
  // the bound tree created by binder when it expands the view text.
  // Adds to baseTableNameList_ the table names of all the Scan
  // nodes that are part of this tree.
  void addTableNameToList(RelExpr * node);

  TableDescList tabIds_;

};

// -----------------------------------------------------------------------
// class RelTransaction
//
// This class implements the BEGIN WORK, COMMIT WORK, ROLLBACK WORK,
// ROLLBACK WORK WAITED, and SET TRANSACTION <options> statements.
//
// Note: All non-generator methods are defined in file RelDCL.C in
//       the optimizer directory.
//       All generator methods are defined in file GenRelDCL.C in
//       the generator directory.
// -----------------------------------------------------------------------
class RelTransaction : public RelExpr
{
public:
  // constructor
  RelTransaction(TransStmtType type, TransMode * mode = NULL,
		 ItemExpr * diagAreaSizeExpr = NULL);

  ~RelTransaction();

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

   // method to do code generation.
  // Defined in GenRelDCL.C in generator directory.
  virtual short codeGen(Generator*);

  // various PC methods
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const{return 0;};
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);
  virtual const NAString getText() const;

  TransStmtType getType() { return type_; }

private:
  // See ComTransInfo.h
  TransStmtType type_;

  // contains options specified in a SET TRANSACTION statement.
  TransMode * mode_;

  // the expression to compute the diagnostic area size
  ItemExpr * diagAreaSizeExpr_;
};

// -----------------------------------------------------------------------
// class RelSetTimeout
//
// This class implements the SET TABLE TIMEOUT statement
//
// Note: All non-generator methods are defined in file RelDCL.cpp in
//       the optimizer directory.
//       All generator methods are defined in file GenRelDCL.cpp in
//       the generator directory.
// -----------------------------------------------------------------------
class RelSetTimeout : public RelExpr
{
public:
  // constructor
  RelSetTimeout(const CorrName & tableName,
		ItemExpr * timeoutValueExpr,
		NABoolean  isStreamTimeout = FALSE,
		NABoolean  isReset = FALSE,
		NABoolean  isForAllTables = FALSE,
		char * physicalFileName = NULL );

  ~RelSetTimeout() {}  // do nothing dtor

  // Copy constructor, not implemented!
  RelSetTimeout (const RelSetTimeout & rel, CollHeap *h=0); 

  // a bunch of virtual methods
  // --------------------------

  // perform name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  // method to do code generation.
  // Defined in GenRelDCL.cpp in generator directory.
  virtual short codeGen(Generator*);

  // various PC methods
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const{return 0;};
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);
  virtual const NAString getText() const;

  //  Public methods unique for SET TABLE TIMEOUT
  //  -------------------------------------------
  // get and set the table name given by the user who called SET TIMEOUT
  inline const CorrName & getUserTableName() const  { return userTableName_; }
  inline       CorrName & getUserTableName()        { return userTableName_; }
  
  ItemExpr * getTimeoutValue()  { return timeoutValueExpr_;}
  NABoolean isStream() { return isStream_; }
  NABoolean isReset() { return isReset_; }
  NABoolean isForAllTables() { return isForAllTables_; }
  void setPhysicalFileName(const char * fName)
  { str_cpy_all( physicalFileName_, fName, 50 ) ; }

private:
  // the user-specified name of the table
  CorrName userTableName_;

  // the expression to compute the value (may be a hostvar or param)
  ItemExpr * timeoutValueExpr_;

  NABoolean isStream_;
  NABoolean isReset_;
  NABoolean isForAllTables_;

  // keep the apropriate guardian file name, given explicitly or deduced
  // by the binder (This is the name used to check statements at runtime.) 
  char physicalFileName_[50];
};

#endif
