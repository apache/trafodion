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
#ifndef EXPRNODE_H
#define EXPRNODE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ExprNode.h
* Description:  Expression nodes (relational nodes and item expression nodes)
*
*               
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "Collections.h"
#include "OperTypeEnum.h"
#include "NAStringDef.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class OperatorType;
class ExprNode;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------

class CacheWA;
class ElemDDLNode;
class RelExpr;
class ItemExpr;
class StmtDDLNode;
class StmtNode;
class TableDesc;

typedef unsigned char CmpPhase;

enum StaticOnly { NOT_STATIC_ONLY = FALSE,
		  STATIC_ONLY_VANILLA = TRUE,
		  STATIC_ONLY_WITH_WORK_FOR_PREPROCESSOR
		};

// -----------------------------------------------------------------------
// class OperatorType: the type of an ExprNode or one of its derived
// class objects. The operator type should uniquely identify the class
// of the actual object used, although a particular class may use more
// than one object type (e.g. a binary arithmetic object class may
// use object types ITM_PLUS, ITM_MINUS, ITM_TIMES, ITM_DIVIDE).
// -----------------------------------------------------------------------
class OperatorType
{
public:

  // conversion from and to OperatorTypeEnum
  OperatorType(OperatorTypeEnum typ) { op_ = typ; }

  inline operator OperatorTypeEnum () const { return op_; }

  // wildcard operators
  NABoolean match(OperatorTypeEnum wildcard) const;
  NABoolean isWildcard() const;

private:

  OperatorTypeEnum op_;
};


// -----------------------------------------------------------------------
//     Operators and their arguments
//     =============================
//     
//     An ExprNode basically represents a node in a tree. The ExprNode
//     has an operator type, which identifies what it does, and it
//     has links to its child nodes (also called input nodes, since
//     the data usually flows up the tree to the root). There is no link
//     to the parent node.
// 
//     An operator's arity is the number of inputs it expects.
//     N-ary operators are permitted; however, any one instance must have
//     a fixed arity.
// -----------------------------------------------------------------------

class ExprNode : public NABasicObject
{
public:

  enum ChildCondition { ALL_CHILDREN,		// all children of this node
  			ANY_CHILD,		// one or more
  			ANY_CHILD_RAW,		// one or more, special format
  			ALL_CHILDREN_RAW,	// all, special format
			EXACTLY_ONE_CHILD,	// one only
			EXACTLY_ONE_CHILD_RAW	// one only, special format
		      };
  
  // default constructor (assuming the max arity in the system is 2)
  ExprNode(OperatorTypeEnum otype = ANY_REL_OR_ITM_OP);

  // copy constructor
  ExprNode(const ExprNode& s);

  // virtual destructor
  virtual ~ExprNode();
  
  // access a child of an ExprNode 
  virtual ExprNode * getChild(Lng32 index);
  virtual const ExprNode * getConstChild(Lng32 index) const;

  // Method for replacing a particular child
  virtual void setChild(Lng32 index, ExprNode *);
  
  // arity of the operator (required number of children)
  virtual Int32 getArity() const; 

  // get and set the type of the operator (usually the type
  // is set by the constructor)
  inline OperatorTypeEnum getOperatorType() const { return operator_; }
  inline const OperatorType & getOperator() const { return operator_; }
  inline void setOperatorType(OperatorTypeEnum newType)
                                              { operator_ = newType; }

  // perform a safe type cast (return NULL ptr for illegal casts)

  virtual ElemDDLNode * castToElemDDLNode();
  virtual const ElemDDLNode * castToElemDDLNode() const;

  virtual RelExpr * castToRelExpr();
  virtual const RelExpr * castToRelExpr() const;

  virtual ItemExpr * castToItemExpr();
  virtual const ItemExpr * castToItemExpr() const;

  virtual StmtDDLNode * castToStmtDDLNode();
  virtual const StmtDDLNode * castToStmtDDLNode() const;

  virtual StmtNode * castToStatementExpr();
  virtual const StmtNode * castToStatementExpr()const;

  // marks that bindNode() has bound the node.
  void markAsBound() { bound_ = TRUE; }

  // marks that bindNode() has bound the node.
  void markAsUnBound() { bound_ = FALSE; }

  // Returns TRUE if bindNode() has already been called on this node.
  NABoolean nodeIsBound() const { return bound_; }

  // --------------------------------------------------------------------
  // transformNode
  //
  // transformNode() is an overloaded name, which is used for a set 
  // of methods that implement the transformation phase of query 
  // normalization. 
  // 
  // We use the term query tree for a tree of relational operators, 
  // each of which can contain none or more scalar expression trees.
  // The transformations performed by transformNode() brings scalar 
  // expressions into a canonical form. The effect of most such
  // transformations is local to the scalar expression tree.
  // However, the transformation of a subquery requires a semijoin
  // to be performed between the relational operator that contains 
  // the subquery and the query tree for the subquery. The effect 
  // of such a subquery transformation is therefore visible not  
  // only in the scalar expression tree but also in the relational
  // expression tree.
  //
  // Originally, transformNode() was a virtual method on ExprNode.
  // The relational as well as scalar expressions are derived from
  // ExprNode and used to provide proprietary implementations for it. 
  // However, relational and scalar expressions have different 
  // processing requirements during the transformation phase. Hence, 
  // each of them now support a virtual transformNode() method that 
  // differ in their interfaces.
  //
  // --------------------------------------------------------------------
  void markAsTransformed() { transformed_ = TRUE; }
  NABoolean nodeIsTransformed() const { return transformed_; }
  void markAsUnTransformed() { transformed_ = FALSE; }
  
  // --------------------------------------------------------------------
  // normalizeNode()
  //
  // normalizeNode() brings a query tree that consists of relational
  // operators into a canonical form. It also rewrites predicates
  // using the transitive closure of values that is computed by 
  // transformNode().
  //
  // This method is invoked on relational as well as item operators.
  //
  // --------------------------------------------------------------------
  void markAsNormalized() { normalized_ = TRUE; }
  NABoolean nodeIsNormalized() const { return normalized_; }

  void markAsSemanticQueryOptimized() { semanticQueryOptimized_ = TRUE; }
  NABoolean nodeIsSemanticQueryOptimized() const { return semanticQueryOptimized_; }
  
  void markAsPreCodeGenned() { preCodeGenned_ = TRUE; }
  void unmarkAsPreCodeGenned() { preCodeGenned_ = FALSE; }
  NABoolean nodeIsPreCodeGenned() const { return preCodeGenned_; }

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  const NAString getTextUpper() const
  { NAString t(getText()); t.toUpper(); return t; }

  // The next method is used to get the text to be used in
  // case of errors -- virtual, so if redefined it can differ from
  // getTextUpper() to return the text for all syntactically equivalent names;
  // e.g., an error while processing UPPER or UCASE, which are
  // processed similarly and are resolved at parse time to the
  // same ItemExpr, would get the text
  //	"UPPER or UCASE".
  //## Yuk -- embedded English text -- this is not per I18N standards!
  virtual const NAString getTextForError() const
  { return getTextUpper(); }

  // print node on a stream
  virtual void print(FILE * f = stdout,
		     const char * prefix = "",
		     const char * suffix = "") const;

  // produce an ascii-version of the object (for display or saving into a file)
  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const ;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  // and add a string describing the function of the child, too
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // display the tree on an X display
  void displayTree();

  enum Cacheability { MAYBECACHEABLE, NONCACHEABLE, 
                      CACHEABLE_PARSE, CACHEABLE_BIND };

  void setNonCacheable() { cacheable_ = NONCACHEABLE; }
  NABoolean isNonCacheable() const { return cacheable_ == NONCACHEABLE; }
  NABoolean maybeCacheable() const { return cacheable_ == MAYBECACHEABLE; }

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa) { return FALSE; }

  void markAsNormalizedForCache() { normalized4Cache_ = TRUE; }
  NABoolean nodeIsNormalizedForCache() const { return normalized4Cache_; }
  
private:

  // specify the type of the operator (join, union, and, or, ...)
  OperatorType  operator_;

  // TRUE if bindNode() has already been called on this node.
  NABoolean     bound_;

  // TRUE if transformNode() has already been called on this node.
  NABoolean     transformed_;

  // TRUE if normalizeNode() has already been called on this node.
  NABoolean     normalized_;

  // TRUE if preCodeGen() has already been called on this node.
  NABoolean     preCodeGenned_;

  // TRUE if normalizeForCache() has already been called on this node.
  NABoolean     normalized4Cache_;

  // TRUE if semanticQueryOptimizeNode() has already been called on this node.
  NABoolean     semanticQueryOptimized_;



 protected:
  Cacheability  cacheable_; // records the cacheability of an ExprNode.
  // most ExprNodes start in the MAYBECACHEABLE state.
  // certain ExprNodes (eg, ControlAbstractClass, Describe, StmtNode, 
  // ElemDDLNode, RelLock, RelTransaction) are always NONCACHEABLE.
  // (tuple) Inserts are CACHEABLE_PARSE (cacheable after parse).
  // some Deletes, Updates, Scans are CACHEABLE_BIND (cacheable after bind).
}; // ExprNode

#endif /* EXPRNODE_H */

