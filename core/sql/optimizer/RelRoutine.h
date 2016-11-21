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
#ifndef RELROUTINE_H
#define RELROUTINE_H
/* -*-C++-*-
**************************************************************************
*
* File:         RelRoutine.h
* Description:  RelExprs related to support for Routines and UDFs
* Created:      6/29/09
* Language:     C++
*
*************************************************************************
*/


#include "ObjectNames.h"
#include "RelExpr.h"
#include "OptUtilIncludes.h"
#include "BinderUtils.h"
#include "StmtNode.h"
#include "RoutineDesc.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------

class RelRoutine;
class TableValuedFunction;
class BuiltinTableValuedFunction;
class TableValuedUDF;
class ExplainFunc;
class PhysicalExplain;
class StatisticsFunc;
class ProxyFunc;
class SPProxyFunc;
class PhysicalSPProxyFunc;
class ExtractSource;
class PhysicalExtractSource;
class IsolatedNonTableUDR;
class CallSP;
class IsolatedScalarUDF;
class SPDupVarList;
class TableMappingUDF;
class TableMappingUDFChildInfo;
class PredefinedTableMappingFunction;
class PhysicalTableMappingUDF;

// forward references
class TMUDFDllInteraction;
namespace tmudr {
  class UDRInvocationInfo;
  class UDRPlanInfo;
  class UDR;
}

typedef Int32 CliRoutineHandle;

// -----------------------------------------------------------------------
/*!
*  \brief RelRoutine Class.
*         The RelRoutine class is the base class for all Routines/Builtin
*         functions, UDFs and Stored Procedure Calls.
*/
// -----------------------------------------------------------------------

class RelRoutine : public RelExpr
{
public:

  // constructors
  
  //! RelRoutine Constructor
  RelRoutine(ItemExpr *params = NULL,
             OperatorTypeEnum otype = REL_ROUTINE,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(otype, NULL, NULL, oHeap),
    procAllParamsTree_(params),
    hasSubquery_(FALSE),
    arity_(0),
    routined_(NULL)
  { }

    RelRoutine(RelExpr * child0,
              ItemExpr *params = NULL,
             OperatorTypeEnum otype = REL_ROUTINE,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(otype, child0, NULL, oHeap),
    procAllParamsTree_(params),
    hasSubquery_(FALSE),
    arity_(1),
    routined_(NULL)
  { }

  //! RelRoutine Constructor
  //  expects at least a name 
  RelRoutine(QualifiedName name, ItemExpr *params = NULL,
             OperatorTypeEnum otype = REL_ROUTINE,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(otype, NULL, NULL, oHeap),
    procAllParamsTree_(params),
    routineName_(name, oHeap),
    hasSubquery_(FALSE),
    arity_(0),
    routined_(NULL)
  { }


  //! RelRoutine Constructor
  //  expects at least a name and an output parameter list.
  RelRoutine(QualifiedName name, ValueIdList outParams, 
             ItemExpr *params = NULL,
             OperatorTypeEnum otype = REL_ROUTINE,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(otype, NULL, NULL, oHeap),
    procAllParamsTree_(params),
    routineName_(name),
    procOutputParamsVids_(outParams),
    hasSubquery_(FALSE),
    arity_(0),
    routined_(NULL)
  { }

  //! Copy Constructor
  //  expects at least a reference to another RelRoutine instance.
  RelRoutine(const RelRoutine &other, 
             CollHeap *oHeap = CmpCommon::statementHeap());

  // Desctructors

  //! ~RelRoutine Destructor
  virtual ~RelRoutine();


  // accessor methods

  //! getRoutineDesc acessor method 
  //  returns a constant reference to the routineDesc
  inline RoutineDesc * getRoutineDesc() const { return routined_; }

  //! getNARoutine acessor method 
  //  returns a constant reference to the NARoutine struct 
  inline const NARoutine * getNARoutine() const
     { return routined_ ? routined_->getNARoutine() : NULL; }

  //! getEffectiveNARoutine acessor method 
  //  returns a constant reference to the Effective NARoutine struct 
  inline const NARoutine * getEffectiveNARoutine() const
     { return routined_ ? routined_->getEffectiveNARoutine() : NULL; }

  //! getRoutineName acessor method 
  //  returns a reference to the routine Name
  inline QualifiedName &getRoutineName() { return routineName_; }

  /* This can be uncommented, perhaps for Seaquest, when the
     QualifiedName class supports namespaces
  inline const ComAnsiNameSpace getRoutineNameSpace() const { return routineName_.getObjectNameSpace(); }
  */

  //! getRoutineName acessor method 
  //  returns a constant reference to the routine Name
  inline const QualifiedName &getRoutineName() const { return routineName_; }

  //! getProcOutputParamsVids acessor method 
  // return a constant reference to the output parameters
  inline const ValueIdList &getProcOutputParamsVids() const 
         { return procOutputParamsVids_; }

  //! getProcOutputParamsVids acessor method 
  // return a reference to the output parameters
  inline ValueIdList &getProcOutputParamsVids() 
         { return procOutputParamsVids_; }

  //! getProcInputParamsVids acessor method 
  // return a constant reference to the input parameters
  inline const ValueIdList &getProcInputParamsVids() const 
         { return procInputParamsVids_; }

  //! getProcInputParamsVids acessor method 
  // return a reference to the input parameters
  inline ValueIdList &getProcInputParamsVids() 
         { return procInputParamsVids_; }

  //! getProcAllParamsVids acessor method 
  // return a reference to all parameters
  inline ValueIdList & getProcAllParamsVids() 
         { return procAllParamsVids_; } 

  //! getProcAllParamsVids acessor method 
  // return a constant reference to all parameters
  inline const ValueIdList & getProcAllParamsVids() const 
         { return procAllParamsVids_; }

  //! getProcAllParamsTree acessor method 
  // return a constant reference to the ItemExpr parameter tree
  inline const ItemExpr * getProcAllParamsTree() const 
         { return procAllParamsTree_; }

  //! getProcAllParamsTree acessor method 
  // return a reference to the ItemExpr parameter tree
  inline ItemExpr * getRWProcAllParamsTree() 
         { return procAllParamsTree_; }

  //! getArity acessor method 
  // for the most part routines don't have children. If they do,
  // they will implement their own getArity() method.
  virtual  Int32 getArity () const { return  arity_ ; }

  virtual void setArity(Int32 val) { arity_ = val; }

  //! hasSubquery method
  //  Does this Routine have parameters containing subqueries?
  inline NABoolean hasSubquery () const { return hasSubquery_; }

  //! hasSubquery method
  //  Does this Routine have parameters containing subqueries?
  inline NABoolean &hasSubquery () { return hasSubquery_; }

  // For MVs - in the future when UDFs can be ALTERed for library 
  // changes, ... this will not always return TRUE.
  virtual NABoolean isIncrementalMV() { return TRUE; }

  // mutator methods

  //! setRoutineDesc mutator method 
  //  initializes the RoutineDesc member to the given parameter.
  inline void setRoutineDesc(RoutineDesc * rdesc) { routined_ = rdesc; }

  //! setRoutineName mutator method 
  //  initializes the Routine Name member to the given parameter.
  inline void setRoutineName( QualifiedName &name) { routineName_ = name; }

  /* This can be uncommented, perhaps for Seaquest, when the
     QualifiedName class supports namespaces
  inline void setRoutineNameSpace(ComAnsiNameSpace nameSpace) { routineName_.setObjectNameSpace(nameSpace); }
  */

  //! setProcAllParamsVids mutator method 
  //  initializes the procAllAParamsVids member to the given parameter.
  inline void setProcAllParamsVids (ValueIdList &paramIds) 
         { procAllParamsVids_ = paramIds; }

  //! setProcAllParamsTree mutator method 
  //  initializes the procAllParamsTree member to the given parameter.
  inline void setProcAllParamsTree (ItemExpr *paramTree) 
         { procAllParamsTree_ = paramTree; }

  //! addProcOutputParamsVids mutator method 
  //  add a ValueId to the procOutputParamsVids list
  inline void addProcOutputParamsVid(ValueId vId) 
         { procOutputParamsVids_.insert(vId); }

  //! setHasSubquery method
  //  Does this Routine have parameters containing subqueries?
  inline void setHasSubquery () { hasSubquery_ = TRUE; }



  //! bindNode method 
  //  a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  //! transformNode method
  // NormRelExpr.cpp
  virtual void transformNode (NormWA & normWARef,
                              ExprGroupId & locationOfPointerToMe);
  //! rewriteNode method 
  //  a virtual function for performing rewriting of expressions during 
  //  normalization phase.
  virtual void rewriteNode(NormWA & normWARef);

  //! synthLogProp method 
  //  a virtual function for type synthesis
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  //! preCodeGen method 
  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);




  //! getPotentialOutputValues method 
  //  method to compute the potential outputs of this RelExpr 
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  //! recomputeOuterReferences method 
  //  method to recompute the outer references this RelExpr needs.
  virtual void recomputeOuterReferences();

  //! addLocalExpr method
  //  Add all the expressions that are local to this node to an
  //  existing list of expressions (used by GUI tool). addLocalExpr()
  //  will call the virtual getInputExpressionName() method to
  //  determine the name of the input expression.
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;


  //! getInputExpressionName method
  //  Used by the GUI tool to display the expression
  virtual const char *getInputExpressionName() const
        { return "function_parameters"; }

  //! getInputExpressionName method
  //  method used by the optimizer to generate a hash for this node
  virtual HashValue topHash();

  //! getInputExpressionName method
  //  method used by the optimizer to check for a duplicate node
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  //! getInputExpressionName method
  //  method used to copy most of a node. generally GA are left out.
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! getText method
  //  method used to display the name of the node
  virtual const NAString getText() const;

  //! pilotAnalysis method
  //  method used by the analyzer. Only needed for classes that results
  //  in a JBBC. Right now TableValuedFunctions are the only ones using it.
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);

  //! countSuppliedParms method
  //  method used to figure out the number of params in the given ItemExpr tree 
  //  This is a new method that perhaps should be moved to ItemExpr class.
  virtual CollIndex countSuppliedParams (ItemExpr *tree) const;

  //! gatherParamValueIds method
  //  method used to harvest valueIds form the given ItemExpr tree 
  //  This is a new method that perhaps should be moved to ItemExpr class.
  virtual void gatherParamValueIds (const ItemExpr *tree, ValueIdList &paramsList) const;

protected:
  inline ItemExpr * getRWProcParams ();

private:

     //! hasSubquery_ member
     //  flag to indicate if the node refers to any subqueries
  NABoolean  hasSubquery_;

  Int32 arity_ ; 

     //! procAllparamsTree_ member
     //  an expression with the Routine's actual parameters
     //  as provided by the parser, if the class got instantiated 
     //  from the parser.
     //  This is used with Routines like Explain etc
     //  Holds all the parameters given to the routine, regardless if 
     //  they are purely input, in/out or output parameters.
     //  For UDFs this list will always only contain inputs.
     //  This list should be set to NULL after binding.
  ItemExpr   *procAllParamsTree_;

     //! procInputParamsVids_ member
     //  List of valueIds representing our input parameters 
  ValueIdList procInputParamsVids_;

     //! procAllParamsVids_ member
     //  List of valueIds containing the above parameters
  ValueIdList procAllParamsVids_;

     //! procOutputParamsVids_ member
     //  List of valueIds representing the output of the Routine
     //  These values are used temporarily to compare outputs to those
     //  stored in the routineDesc.
  ValueIdList procOutputParamsVids_;

     //! routined_ member
     //  Pointer to the RoutineDesc. Primary function for the RoutineDesc
     //  is to hold the pointers to the NAroutine structs and our outputs
  RoutineDesc *routined_;

     //! routineName_ member
     //  Name of the routine. We also have a copy of the name in the RoutineDesc
     //  and in the NARoutine structs.
  QualifiedName routineName_;

}; // class RelRoutine


// -----------------------------------------------------------------------
/*!
*  \brief TableValuedFunction Class.
*         The TableValuedFunction Class is the base class for all TableValued
*         Routines/Builtinfunctions, functions, UDFs and Stored Procedure Calls.
*/
// -----------------------------------------------------------------------


class TableValuedFunction : public RelRoutine
{
public: 
  // constructors

  //! TableValuedFunction Constructor with no children
  //  can be instantiated without any parameters
  TableValuedFunction(ItemExpr *params = NULL,
                      OperatorTypeEnum otype = REL_TABLE_VALUED_FUNCTION,
                      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelRoutine(params, otype, oHeap),
    userTableName_("", oHeap),
    tabId_(0)
  { }

  //! TableValuedFunction Constructor with 1 child
  //  can be instantiated without any parameters
  TableValuedFunction(RelExpr * child0,
                      ItemExpr *params = NULL,
                      OperatorTypeEnum otype = REL_TABLE_VALUED_FUNCTION,
                      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelRoutine(child0, params, otype, oHeap),
    userTableName_("", oHeap),
    tabId_(0)
  { }

  //! TableValuedFunction Constructor
  //  wants at least a name for the routine.
  TableValuedFunction(const CorrName &name, ItemExpr *params = NULL,
                      OperatorTypeEnum otype = REL_TABLE_VALUED_FUNCTION,
                      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelRoutine(name.getQualifiedNameObj(), params, otype, oHeap),
    userTableName_(name, oHeap),
    tabId_(0)
  { }

  //! TableValuedFunction Copy Constructor
  //  wants at least a name for the routine.
  TableValuedFunction(const TableValuedFunction &other,
                      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelRoutine(other, oHeap),
    tabId_(other.tabId_),
    userTableName_(other.userTableName_) 
  { }


  // destructors

  //! ~TableValuedFuntion method
  //  destroys the instance
  virtual ~TableValuedFunction();

  // accessor methods

  //! getVirtualTableName method
  //  returns a const char string for the Virtual Table Name
  // should return a CorrName?##
  virtual const char *getVirtualTableName();  

  //! getTableDesc method
  //  returns a pointer to a TableDesc member
  inline TableDesc * getTableDesc() const { return tabId_; }

  // mutator methods

  //! setTableDesc method
  //  sets the TableDesc member to the given value
  inline void setTableDesc(TableDesc * tdesc) { tabId_ = tdesc; };

  //! createVirtualTableDesc method
  //  creates a TrafDesc for the Virtual Table
  virtual TrafDesc *createVirtualTableDesc();

  //! deleteVirtualTableDesc method
  //  deletes the given TrafDesc
  virtual  void deleteVirtualTableDesc(TrafDesc *vtd);

  //! bindNode method
  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  //! getPotentialOutputValues method
  // a virtual function for computing the potential outputValues
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  //! getInputExpressionName method
  // a virtual function used by the GUI for displaying the input expression
  virtual const char *getInputExpressionName() const
  { return "TableValuedFunction inputs"; }


  //! duplicateMatch method
  // a virtual function used by the optimizer to detect indentical nodes
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  //! copyTopNode method
  // a virtual function used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! getText method
  // a virtual function used to display the name of the node
  const NAString getText() const ;

  //! pilotAnalysis method
  // a virtual function used by the analyzer
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);

    // get and set the table name
  const CorrName & getUserTableName() const   { return userTableName_; }
  CorrName & getUserTableName()               { return userTableName_; }
  void setUserTableName(const CorrName &userTableName)
  {
    userTableName_ = userTableName;
  }
  // used only in the parser to override udf name is AS clause was used
  void setCorrName(const NAString& corrName)
  {
    getUserTableName().setCorrName(corrName);
  }

private:

  //! tabId_
  // a unique identifier for the virtual table
  TableDesc * tabId_;

  CorrName userTableName_;
}; // class TableValuedFunction



// -----------------------------------------------------------------------
/*!
*  \brief BuiltinTableValuedFunction Class.
*         The BuiltinTableValuedFunction Class is the base class for all 
*         builtin TableValued Routines/Builtinfunctions, functions, UDFs and 
*         Stored Procedure Calls.
* This class has no members and is used as an abstract to isolate out
* inheritance and concepts.
*/
// -----------------------------------------------------------------------


class BuiltinTableValuedFunction: public TableValuedFunction 
{

public:
  // constructors

  //! BuiltinTableValuedFunction Constructor 
  BuiltinTableValuedFunction(ItemExpr *params = NULL,
              OperatorTypeEnum otype = REL_BUILTIN_TABLE_VALUED_FUNCTION,
              CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(params, otype, oHeap)
  {
    // We don't want explain et al to be cached.
    setNonCacheable();
  }

  //! BuiltinTableValuedFunction Constructor 
  //  expects at least a name
  BuiltinTableValuedFunction(const CorrName &name, ItemExpr *params = NULL,
              OperatorTypeEnum otype = REL_BUILTIN_TABLE_VALUED_FUNCTION,
              CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(name, params, otype, oHeap)
  {
    // We don't want explain et al to be cached.
    setNonCacheable();
  }

  //! BuiltinTableValuedFunction Copy Constructor 
  //  Copy constructor 
  BuiltinTableValuedFunction(const BuiltinTableValuedFunction &other,
                             CollHeap *h = CmpCommon::statementHeap()) 
  : TableValuedFunction(other, h)
  {}
                  

  //! ~BuiltinTableValuedFunction() destructor method
  // a virtual function used to create an Almost complete copy of the node
  virtual ~BuiltinTableValuedFunction();


  //! bindNode method
  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  //! getText method
  // a virtual function for displaying the name of the node
  virtual const NAString getText() const ;

  virtual Lng32 numParams() { return 2; }

private:

   // No members

}; // class BuiltinTableValuedFunction

class TableMappingUDFChildInfo : public NABasicObject
{

  friend class TableMappingUDF ;

  public:

  TableMappingUDFChildInfo(
              const NAString& name,
              NAColumnArray& inputCols,
              TMUDFInputPartReq partType, 
              ValueIdSet& partBy,
              ValueIdList& orderBy,
              ValueIdList& outputs,
              CollHeap *oHeap = CmpCommon::statementHeap())
              : inputTabName_(name),
                inputTabCols_(inputCols),
                partType_(partType),
                partitionBy_(partBy),
                orderBy_(orderBy),
                outputs_(outputs)
               {}

  TableMappingUDFChildInfo(
                TMUDFInputPartReq partType, 
                ValueIdSet& partBy,
                ValueIdList& orderBy,
                CollHeap *oHeap = CmpCommon::statementHeap())
                : partType_(partType),
                  partitionBy_(partBy),
                  orderBy_(orderBy)
                {}

  TableMappingUDFChildInfo(const TableMappingUDFChildInfo &other);

  const NAString& getInputTabName()           {return inputTabName_;}
  const NAColumnArray& getInputTabCols()      {return inputTabCols_;}
  TMUDFInputPartReq getPartitionType()        {return partType_;}
  void setPartitionType(TMUDFInputPartReq val){partType_ = val;}
  const ValueIdSet & getPartitionBy() const   {return partitionBy_; }
  void setPartitionBy(const ValueIdSet& val)  {partitionBy_ = val; }
  const ValueIdList & getOrderBy() 	      {return orderBy_; }
  void setOrderBy(const ValueIdList & val)    {orderBy_ = val; }
  const ValueIdList & getOutputs() 	      {return outputs_; }
  ValueIdList & getOutputIds() 	              {return outputs_; }
  void removeColumn(CollIndex i);

  private :
    NAString inputTabName_;
    NAColumnArray inputTabCols_; // contains input col name and type
    TMUDFInputPartReq partType_;
    ValueIdSet partitionBy_;
    ValueIdList orderBy_;
    ValueIdList outputs_; // we need to know the actual order in which the 
                          // TMUDF needs the child output to be arranged
};

// -----------------------------------------------------------------------
/*!
*  \brief TableMappingUDF Class.
*         The TableMappingUDF Class represents the Table Mapping UDF object
*         It is a table valued UDF in that produces a table as output
*         It is different from a table valued UDF in that it has one or more
*         table expressions as input. It also has scalar expressions as input
*         just like all other UDFs.
*/
// -----------------------------------------------------------------------

class TableMappingUDF: public TableValuedFunction 
{
public :
  // constructors

  //! TableMappingUDF Constructor
  TableMappingUDF(ItemExpr *params = NULL,
                 OperatorTypeEnum otype = REL_TABLE_MAPPING_UDF,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(params, otype, oHeap),
  childInfo_(oHeap),
  scalarInputParams_(NULL),
  outputParams_(NULL),
  dllInteraction_(NULL),
  invocationInfo_(NULL),
  numPlanInfos_(0),
  routineHandle_(NullCliRoutineHandle)
  { };

  TableMappingUDF(RelExpr * child0,
                  ItemExpr *params = NULL,
                 OperatorTypeEnum otype = REL_TABLE_MAPPING_UDF,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(child0, params, otype, oHeap),
    childInfo_(oHeap),
    scalarInputParams_(NULL),
    outputParams_(NULL),
    dllInteraction_(NULL),
    invocationInfo_(NULL),
    numPlanInfos_(0),
    routineHandle_(NullCliRoutineHandle)
  { };
  //! TableMappingUDF Constructor
  //  expects at least a name
  TableMappingUDF(const CorrName &name, ItemExpr *params = NULL,
                 OperatorTypeEnum otype = REL_TABLE_MAPPING_UDF,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(name, params, otype, oHeap),
    childInfo_(oHeap),
    scalarInputParams_(NULL),
    outputParams_(NULL),
    dllInteraction_(NULL),
    invocationInfo_(NULL),
    numPlanInfos_(0),
    routineHandle_(NullCliRoutineHandle)
  { };

  //! TableValueUDF Copy Constructor 
  // a virtual function for displaying the name of the node
  TableMappingUDF(const TableMappingUDF &other);

  // destructors

  //! ~TableValuedUDF destructor
  virtual ~TableMappingUDF();
  //! copyTopNode method
  // a virtual function used to copy most of a Node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual TableMappingUDF *castToTableMappingUDF();

  //! getText method
  // a virtual function for displaying the name of the node
  virtual const NAString getText() const ;

  // safe method to cast to derived class
  virtual PredefinedTableMappingFunction * castToPredefinedTableMappingFunction();

  virtual RelExpr* bindNode(BindWA* bindWA); 
  virtual TrafDesc   *createVirtualTableDesc();
  virtual void transformNode(NormWA & normWARef,
    ExprGroupId & locationOfPointerToMe);
  virtual void rewriteNode(NormWA & normWARef) ;
 
  virtual void primeGroupAnalysis();

  // --------------------------------------------------------------------
  // A method that provides the set of values that can potentially be
  // produced as output by this operator. These are values over and
  // above those that are prodcued as Characteristic Outputs.
  // --------------------------------------------------------------------
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;
  virtual void getPotentialOutputValuesAsVEGs(ValueIdSet& outputs) const;
  virtual void pullUpPreds();
  virtual void recomputeOuterReferences();

  virtual void pushdownCoveredExpr(
                                   const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet & newExternalInputs,
                                   ValueIdSet& predOnOperator,
				   const ValueIdSet *
				      nonPredNonOutputExprOnOperator = NULL,
                                   Lng32 childId = (-MAX_REL_ARITY));

  // *********************************************************************
  // Helper functions and methods used by the optimizer.
  // *********************************************************************

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  // finish the synthesis of logical properties by setting various cardinality
  // related attributes
  virtual void finishSynthEstLogProp();

  // synthesize estimated logical properties given a specific set of
  // input log. properties
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputLP);

  ///////////////////////////////////////////////////////////

  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  virtual void jbbAnalysis(QueryAnalysis* qa);
  virtual void jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies);
  virtual void predAnalysis(QueryAnalysis* qa);
  virtual RelExpr* convertToMultiJoinSubtree(QueryAnalysis* qa);
  virtual RelExpr* expandMultiJoinSubtree();

  ///////////////////////////////////////////////////////////

  // do some analysis on the initial plan
  // this is called at the end of the analysis phase
  virtual void analyzeInitialPlan();

  // ---------------------------------------------------------------------
  // comparison, hash, and copy methods
  // ---------------------------------------------------------------------

  virtual SimpleHashValue hash(); // from ExprNode
  virtual HashValue topHash();
  virtual NABoolean patternMatch(const RelExpr & other) const;
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  // --------------------------------------------------------------------
  // Methods used internally by Cascades
  // --------------------------------------------------------------------

  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const;
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const;

  // ---------------------------------------------------------------------
  // A virtual function that decides if ESP parallelism is allowed
  // by the settings in the defaults table, or if the number of
  // ESPs is being forced.
  // ---------------------------------------------------------------------
  virtual DefaultToken getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const ;

  // ---------------------------------------------------------------------
  // A virtual function that decides if ESP parallelism is worth it
  // for the operator.
  // ---------------------------------------------------------------------
  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;

  // ---------------------------------------------------------------------
  // Performs mapping on the partitioning function, from this
  // operator to the designated child, if the operator has/requires mapping.
  // Note that this default implementation does no mapping.
  // ---------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;


  // ---------------------------------------------------------------------
  // for debugging
  // ---------------------------------------------------------------------

  // add all the expressions that are local to this
  // node to an existing list of expressions
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;


  virtual NABoolean isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 /*planNumber*/);

  void checkAndCoerceScalarInputParamTypes(BindWA* bindWA);

  void createOutputVids(BindWA* bindWA);

  TableMappingUDFChildInfo * getChildInfo(Int32 index)
  {
    CMPASSERT (index >= 0 && index < (Int32) childInfo_.entries()); 
    return childInfo_[index];
  }

  TMUDFDllInteraction * getDllInteraction()
  {
    return dllInteraction_;
  }

  tmudr::UDRInvocationInfo * getInvocationInfo()
  {
    return invocationInfo_;
  }

  int getNextPlanInfoNum()
  {
    return numPlanInfos_++;
  }

  char * getConstParamBuffer() const
  {
    return constParamBuffer_;
  }

  Int32 getConstParamBufferLen() const
  {
    return constParamBufferLen_;
  }

  CliRoutineHandle getRoutineHandle() const
  {
    return routineHandle_;
  }

  inline  const NAColumnArray    &getScalarInputParams()   const 
  { return *scalarInputParams_; }

  inline       ComSInt32       getScalarInputParamCount()  const 
  { return scalarInputParams_->entries();}

  inline void setScalarInputParams(NAColumnArray* val)
  {	scalarInputParams_ = val;}

  inline  const NAColumnArray    &getOutputParams()        const 
  { return *outputParams_;  }

  inline void setOutputParams(NAColumnArray* val)
  {	outputParams_ = val;}

  inline void removeOutputParam(CollIndex i)
  {	outputParams_->removeAt(i);
        getProcOutputParamsVids().removeAt(i); }

  inline       ComSInt32        getOutputParamCount()     const 
  { return outputParams_->entries();}

  inline void setInvocationInfo(tmudr::UDRInvocationInfo *invocationInfo)
  { invocationInfo_ = invocationInfo; }

  inline void setConstParamBuffer(char *buf, int len)
  { constParamBuffer_ = buf; constParamBufferLen_ = len; }

  inline void setRoutineHandle(CliRoutineHandle handle)
  { routineHandle_ = handle; }

protected:

  virtual NARoutine * getRoutineMetadata(QualifiedName &routineName,
                                         CorrName &tmfuncName,
                                         BindWA *bindWA);

  LIST(TableMappingUDFChildInfo *) childInfo_ ;

  // the udrInterface should be in a cache, created when we invoke
  // a particular interface for the first time and deleted when
  // we unload the DLL
  CliRoutineHandle routineHandle_;

  ValueIdSet predsEvaluatedByUDF_;

  // selectivity hint from user. Set in the parser. Set if > 0, set -1 otherwise
  CostScalar selectivityFactor_;
  CostScalar cardinalityHint_;

  NAColumnArray* scalarInputParams_;
  NAColumnArray* outputParams_;
  
  // a map that defines the mapping from input
  // columns of the udf to its output columns
  // top values: udf output
  // bottom values: child(ren) input
  ValueIdMap udfOutputToChildInputMap_;
  
  // objects needed for the interaction with TMUDFs at compile time
  TMUDFDllInteraction *dllInteraction_ ;
  tmudr::UDRInvocationInfo *invocationInfo_;
  int numPlanInfos_;
  char *constParamBuffer_;
  Int32 constParamBufferLen_;

}; // class TableMappingUDF

class PredefinedTableMappingFunction : public TableMappingUDF
{
  // Predefined table mapping functions behave like UDFs and are
  // implemented like UDFs, using the same interfaces, but they
  // are created by the system and are always available. The DLL
  // that implements them is distributed with the system.
  // So, "predefined" is different from "builtin" in that builtin
  // functions have their own executor operators and work
  // methods, while predefined functions are executed like
  // UDFs at runtime and during most of compile time.
  // The physical node for a PredefinedTableMappingFunction
  // is a PhysicalTableMappingUDF, like for regular TMUDFs.
public:
  PredefinedTableMappingFunction(
       const CorrName &name,
       ItemExpr *params,
       OperatorTypeEnum otype,
       CollHeap *oHeap = CmpCommon::statementHeap());

  virtual ~PredefinedTableMappingFunction();

  // static method to find out whether a given name is a
  // built-in table-mapping function - returns operator type
  // of predefined function if so, REL_TABLE_MAPPING_UDF otherwise
  static OperatorTypeEnum nameIsAPredefinedTMF(const CorrName &name);

  virtual const NAString getText() const;

  virtual PredefinedTableMappingFunction * castToPredefinedTableMappingFunction();

  // a virtual function used to copy most of a Node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

private:

  virtual NARoutine * getRoutineMetadata(QualifiedName &routineName,
                                         CorrName &tmfuncName,
                                         BindWA *bindWA);

};

// -----------------------------------------------------------------------
/*!
*  \brief PhysicalTableMappingUDF Class.
*         The PhysicalTableMappingUDF replaces the logical TableMappingUDF 
*         through the application of the PhysicalTableMappingUDF.
*         This transformation is designed to present a purely
*          physical verison of an operator
*         that is both logical and physical.
*/
// -----------------------------------------------------------------------
class PhysicalTableMappingUDF : public TableMappingUDF
{
public:
  // constructor

  //! PhysicalTableMappingUDF Constructor
  PhysicalTableMappingUDF(CollHeap *oHeap = CmpCommon::statementHeap())
       : TableMappingUDF(NULL, REL_TABLE_MAPPING_UDF, oHeap),
         planInfo_(NULL)
  {}

  PhysicalTableMappingUDF(RelExpr* child0,
                          CollHeap *oHeap = CmpCommon::statementHeap())
       : TableMappingUDF(child0, NULL, REL_TABLE_MAPPING_UDF, oHeap),
         planInfo_(NULL)
  {}

  //! PhysicalTableMappingUDF Copy Constructor
  PhysicalTableMappingUDF(const TableMappingUDF & other)
       : TableMappingUDF(other),
         planInfo_(NULL)
  {}

  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const { return FALSE; }
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const { return TRUE; }

  virtual PlanWorkSpace* allocateWorkSpace() const;
  // ---------------------------------------------------------------------
  // Create a Context for a specific child and store it in the
  // PlanWorkSpace.
  // This method is called from within the implementation
  // of createPlan().
  // ---------------------------------------------------------------------
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  // ---------------------------------------------------------------------
  // Obtain a pointer to a CostMethod object that provides access
  // to the cost estimation functions for nodes of this type.
  // ---------------------------------------------------------------------
  virtual CostMethod* costMethod() const;

  // ---------------------------------------------------------------------
  // calculate physical properties from child's phys properties
  // (assuming the children are already optimized and have physical properties)
  // (used in the implementation of createPlan)
  // ---------------------------------------------------------------------
  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  virtual double getEstimatedRunTimeMemoryUsage(ComTdb * tdb) ;

  virtual short codeGen(Generator *);

  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					    ComTdb * tdb,
					    Generator *generator);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! getText method
  // a virtual function for displaying the name of the node
  virtual const NAString getText() const ;

private:

  // this only gets set during preCodeGen, in the optimizer this is
  // stored in the TMUDFPlanWorkSpace
  tmudr::UDRPlanInfo *planInfo_;

}; // class PhysicalTableMappingUDF



// -----------------------------------------------------------------------
/*!
*  \brief TableValuedUDF Class.
*         The TableValuedUDF Class is the base class for all UDFs 
*
* This class will be implemented in the next phase of the UDF project
*/
// -----------------------------------------------------------------------

class TableValuedUDF: public TableValuedFunction 
{
  // constructors

  //! TableValuedUDF Constructor
  TableValuedUDF(ItemExpr *params = NULL,
                 OperatorTypeEnum otype = REL_TABLE_VALUED_UDF,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(params, otype, oHeap)
  { }

  //! TableValuedUDF Constructor
  //  expects at least a name
  TableValuedUDF(const CorrName &name, ItemExpr *params = NULL,
                 OperatorTypeEnum otype = REL_TABLE_VALUED_UDF,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : TableValuedFunction(name, params, otype, oHeap)
  { }

  // destructors

  //! ~TableValuedUDF destructor
  virtual ~TableValuedUDF();
  //! copyTopNode method
  // a virtual function used to copy most of a Node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! getText method
  // a virtual function for displaying the name of the node
  virtual const NAString getText() const ;

private:

  //! TableValueUDF Copy Constructor 
  //  Copy constructor without heap is not implemented nor allowed.
  TableValuedUDF(const TableValuedUDF &other);


   // No members for now

}; // class TableValuedUDF

// -----------------------------------------------------------------------
/*!
*  \brief ExplainFunc Class.
*         The Explain class represents a table containing the explain info
*         of the named modules and statements.
*/
// -----------------------------------------------------------------------
class ExplainFunc : public BuiltinTableValuedFunction
{
public:

  // constructor

  //! ExplainFunc Constructor
  ExplainFunc(ItemExpr *params = NULL,
              CollHeap *oHeap = CmpCommon::statementHeap())
  : BuiltinTableValuedFunction(params,REL_EXPLAIN,oHeap)
  {}

  // destructors


  //! ~ExplainFunc Destructor
  virtual ~ExplainFunc();


  // acessors
  //! getVirtualTableName method
  //  returns a const char pointer to the name of the virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName();  
  static const char * getVirtualTableNameStr() { return "EXPLAIN__";}

  //! getArity method
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const { return 0; }

  // mutators

  //! createVirtualTableDesc method
  //  creates a TrafDesc for the Virtual Table
  virtual TrafDesc *createVirtualTableDesc();

  //! getPotentialOutputValues method
  //  computes the output values the node can generate
  //  Relies on TableValuedFunctions implementation.

  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);


  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthLogProp method
  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  //! synthEstLogProp method
  // used by the compiler to estimate cost
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  //! synthPhysicalProperty
  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  //! getText method
  //  used to display the name of the node.
  virtual const NAString getText() const;

private:

  //! ExplainFunc Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  ExplainFunc(const ExplainFunc &other);


}; // class ExplainFunc


// -----------------------------------------------------------------------
/*!
*  \brief PhysicalExplain Class.
*         The PhysicalExplain replaces the logical Explain through the
*         application of the PhysicalExplainRule. This transformation is
*         designed to present a purely physical verison of an operator
*         that is both logical and physical.
*/
// -----------------------------------------------------------------------
class PhysicalExplain : public ExplainFunc
{
public:
  // constructor

  //! PhysicalExplain Constructor
  PhysicalExplain(CollHeap *oHeap = CmpCommon::statementHeap())
  : ExplainFunc(NULL, oHeap)
  {}


  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const { return FALSE; }
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const { return TRUE; }

private:

  //! PhysicalExplain Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  PhysicalExplain(const ExplainFunc & other);

}; // class PhysicalExplain


// -----------------------------------------------------------------------
/*!
*  \brief StatisticFunc Class.
*        The Statistics class represents a table containing the explain info
*        of the named modules and statements.
*        This is both a logical and a physical operator.
*/
// -----------------------------------------------------------------------
class StatisticsFunc : public BuiltinTableValuedFunction
{
public:

  // constructor

  //! StatisticsFunc Constructor
  StatisticsFunc(ItemExpr *params = NULL,
                 CollHeap *oHeap = CmpCommon::statementHeap())
  : BuiltinTableValuedFunction(params,REL_STATISTICS,oHeap)
  {}

  // acessors

  //! getVirtualTableName method
  //  returns a const char pointer to the name of the Virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName();
  static const char * getVirtualTableNameStr() { return "STATISTICS__";}

  //! getArity method
  //  get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const { return 0; };

  //! isLogical method
  // this is both logical and physical node
  virtual NABoolean isLogical() const { return TRUE;};

  //! isPhysical method
  // this is both logical and physical node
  virtual NABoolean isPhysical() const { return TRUE;};


  // mutators
  //! createVirtualTableDesc method
  //  returns a TrafDesc pointer to the name of the Virtual Table descriptor
  virtual TrafDesc *createVirtualTableDesc();


  //! preCodeGen method
  //  method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  //! codeGen method
  //  virtual method to do code generation
  virtual short codeGen(Generator*);

  //! copyTopNode method
  //  method to copy Almost a complete node 
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthPyshicalProperty method
  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  //! getText method
  //  get a printable string that identifies the operator
  virtual const NAString getText() const;

private:


}; // class StatisticsFunc

// -----------------------------------------------------------------------
/*!
*  \brief HiveMDaccessFunc Class.
*         The HiveMD class represents a table containing the explain info
*         of the named modules and statements.
*/
// -----------------------------------------------------------------------
class HiveMDaccessFunc : public BuiltinTableValuedFunction
{
public:

  // constructor

  //! HiveMDaccessFunc Constructor
  HiveMDaccessFunc(NAString *mdt = NULL,
                   NAString* schName = NULL,
                   NAString* objName = NULL,
                   CollHeap *oHeap = CmpCommon::statementHeap());

  // destructors

  //! ~HiveMDaccessFunc Destructor
  virtual ~HiveMDaccessFunc();

  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  virtual Lng32 numParams() { return 1; }

  // acessors
  //! getVirtualTableName method
  //  returns a const char pointer to the name of the virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName();  
  static NABoolean isHiveMD(const NAString &name);
  static NAString getMDType(const NAString &name);

  //! getArity method
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const { return 0; }

  // mutators

  //! createVirtualTableDesc method
  //  creates a TrafDesc for the Virtual Table
  virtual TrafDesc *createVirtualTableDesc();

  //! getPotentialOutputValues method
  //  computes the output values the node can generate
  //  Relies on TableValuedFunctions implementation.

  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);


  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthLogProp method
  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  //! synthEstLogProp method
  // used by the compiler to estimate cost
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  //! synthPhysicalProperty
  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  //! getText method
  //  used to display the name of the node.
  virtual const NAString getText() const;

  void setMDtype(NAString mdt) { mdType_ = mdt; }

private:

  //! HiveMDaccessFunc Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  HiveMDaccessFunc(const HiveMDaccessFunc &other);

  NABoolean isHiveSelExpr(ItemExpr * ie, Generator * generator);
  ItemExpr * createHiveSelExpr(ItemExpr * ie, Lng32 &paramNum, 
			       Generator * generator);

  NAString mdType_;

  // Hive schema to be accessed
  // If this is passed in as NULL, then the current default is used.
  NAString schemaName_;

  // hive object name to be accessed. If null, all objects are returned.
  NAString objectName_;

}; // class HiveMDaccessFunc


// -----------------------------------------------------------------------
/*!
*  \brief PhysicalHiveMD Class.
*         The PhysicalHiveMD replaces the logical HiveMD through the
*         application of the PhysicalHiveMDRule. This transformation is
*         designed to present a purely physical verison of an operator
*         that is both logical and physical.
*/
// -----------------------------------------------------------------------
class PhysicalHiveMD : public HiveMDaccessFunc
{
public:
  // constructor

  //! PhysicalHiveMD Constructor
  PhysicalHiveMD(CollHeap *oHeap = CmpCommon::statementHeap())
       : HiveMDaccessFunc(NULL, NULL, NULL, oHeap)
  {}


  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const { return FALSE; }
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const { return TRUE; }

private:

  //! PhysicalHiveMD Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  PhysicalHiveMD(const HiveMDaccessFunc & other);

}; // class PhysicalHiveMD

// -----------------------------------------------------------------------
/*!
*  \brief ProxyFunc Class.
*         The ProxyFunc class represents a virtual table-valued function that
*         at runtime materializes rows whose column types match those in the
*         columnsFromParser list. This is intended to be an abstract
*         class. The two subclasses we have currently are SPProxyFunc which
*         retrieves stored procedure result set rows from an MXUDR server,
*         and ExtractSource which retrieves a parallel extract stream from an
*         MXESP.
*/
// -----------------------------------------------------------------------
class ProxyFunc : public BuiltinTableValuedFunction
{
public:
  // constructors 

  //! ProxyFunc Constructor
  ProxyFunc(OperatorTypeEnum otype,
            ElemDDLNode *columnsFromParser,
            CollHeap *heap);
  
  //! ProxyFunc Copy Constructor
  // Copy constructor 
  ProxyFunc(const ProxyFunc &other, CollHeap * h = CmpCommon::statementHeap());
  // destructors 

  //! ~ProxyFunc Destructor
  virtual ~ProxyFunc();



  // acessors


  //! getNumColumns method
  // Return the number of output columns
  ComUInt32 getNumColumns() const;

  //! getColumnType method
  // Return the datatype for column i
  const NAType &getColumnType(ComUInt32 i) const;

  //! getColumnNameForDescriptor method
  // Return the column name for column i
  const NAString *getColumnNameForDescriptor(ComUInt32 i) const;

  //! getColumnHeading method
  // Return the column heading for column i
  const NAString *getColumnHeading(ComUInt32 i) const;

  //! getTableNameForDescriptor method
  // Return the table name for column i
  const QualifiedName *getTableNameForDescriptor(ComUInt32 i) const;

  // The TableValuedFunction interface

  //! getVirtualTableName method
  //  return a const char pointer to the name of the Virtual Table
  virtual const char *getVirtualTableName();


  // mutators

  //! populateColumnDesc method
  // Copy attributes getColumns to colDescs
void populateColumnDesc(char *tableNam,
                        TrafDesc *&colDescs,
                        Lng32 &reclen) const;

  //! createVirtualTableDesc method
  //  create the TrafDesc for the Virtual Table
  virtual TrafDesc *createVirtualTableDesc();

  // The RelExpr interface
  //! bindNode method
  //  bind the node and its children
  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  //! copyTopNode method
  //  crete an Almost complete copy of the Node
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap* outHeap = 0);
  //! topHash method
  //  used by the optimizer to generate a hash for the node
  virtual HashValue topHash();

  //! duplicateMatch method
  //  used by the optimizer to identify identical nodes
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

protected:
  //! createProxyFuncTableDesc method
  // A helper function used by bindNode() implementations in
  // sublcasses to create a TableDesc from the list of column types.
  void createProxyFuncTableDesc(BindWA *bindWA, CorrName &corrName);

  //! getColumn method
  // We only expose parser structures in the private interface, to
  // prevent use of them in compiler components such as the generator
  const ElemProxyColDef &getColumn(ComUInt32) const;

  //! columnListFromParser_ member
  // We will allow an instance of this class to keep pointers to parse
  // tree structures such as the column definitions. The parse tree
  // should never be modified. By treating the parse tree as read-only
  // it can be shared by other instances of the class created by the
  // copyTopNode() method.
  ElemDDLNode *columnListFromParser_;

  //! virtualTableName_ member
  // Each instance can have a unique table name. If one instance is
  // copied from another with copyTopNode() or the copy constructor,
  // we allow those instances to have the same table name.
  NAString virtualTableName_;

private:
  //! ProxyFunc default Constructor
  // Default constructor without heap is not implemented nor allowed.
  ProxyFunc();


}; // class ProxyFunc

// -----------------------------------------------------------------------
/*!
*  \brief SPProxyFunc Class.
*         The SPProxyFunc class represents a virtual stored procedure result
*         set that at runtime will be retrieved from the UDR server. This is
*         a logical-only operator. The physical counterpart is class
*         PhysicalSPProxyFunc, declared immediately below.
*/
// -----------------------------------------------------------------------
class SPProxyFunc : public ProxyFunc
{
public:
  // constructors

  //! SPProxyFunc Constructor
  SPProxyFunc(ElemDDLNode *columnsFromParser,
              ItemExprList *optionalStrings,
              CollHeap *heap);
  
  //! SPProxyFunc Constructor
  // Default constructor
  SPProxyFunc(CollHeap *oHeap = CmpCommon::statementHeap());

  //! SPProxyFunc Copy Constructor
  // Copy constructor 
  SPProxyFunc(const SPProxyFunc &other, 
              CollHeap *h = CmpCommon::statementHeap());
  // destructors


  //! ~SPProxyFunc Destructor
  virtual ~SPProxyFunc();



  // acessors


  //! getNumOptionalStrings method
  // Return the number of optional strings specified in the
  // syntax. These strings are for internal troubleshooting purposes
  // only.
  ComUInt32 getNumOptionalStrings() const;

  //! getOptionalString method
  // Return the i-th optional string
  const char *getOptionalString(ComUInt32 i) const;

  // The RelExpr interface

  //! getArity method
  //  return the degree of the node
  virtual Int32 getArity() const { return 0; }

  //! isLogical method
  //  indicates if the node is a logical node
  virtual NABoolean isLogical() const { return TRUE; }

  //! isPhysical method
  //  indicates if the node is a physical node
  virtual NABoolean isPhysical() const { return FALSE; }

  //! getText method
  //  Return string used to display the name of the node
  virtual const NAString getText() const;


  //! isCacheableExpr method
  //  indicate if the node can be cached
  virtual NABoolean isCacheableExpr(CacheWA& cwa) { return FALSE; }


  // mutators

  //! addExplainInfo method
  //  creates an ExplainTuple
  ExplainTuple *addExplainInfo(ComTdb *tdb,
                               ExplainTuple *leftChild,
                               ExplainTuple *rightChild,
                               Generator *generator);

  // The RelExpr Interface

  //! topHash method
  //  used by the optimizer to generate a hash for the Node
  virtual HashValue topHash();

  //! copyTopNode method
  //  used to create an Almost complete copy of the node
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap* outHeap = 0);

  //! duplicateMatch method
  //  used by the optimzier to indentify identical nodes
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

private:
  //! optionalStings_ member
  ItemExprList *optionalStrings_;

}; // class SPProxyFunc

// -----------------------------------------------------------------------
/*!
*  \brief PhysicalSPProxyFunc Class.
*/
// -----------------------------------------------------------------------

class PhysicalSPProxyFunc : public SPProxyFunc
{
public:
  // constructors

  //! PhysicalSPProxyFunc Constructor
  PhysicalSPProxyFunc(CollHeap *oHeap = CmpCommon::statementHeap())
    : SPProxyFunc(oHeap)
  {}

  // The physical RelExpr interface
  // acessors

  //! isLogical method 
  //  returns indication if the node is a logical one
  virtual NABoolean isLogical() const { return FALSE; }

  //! isPhysical method 
  //  returns indication if the node is a physical one
  virtual NABoolean isPhysical() const { return TRUE; }

  // mutators

  //! codeGen method 
  //  virtual metod to generate code for the node
  virtual short codeGen(Generator *);

  //! synthPhysicalProperty method 
  //  metod to generate code for the node
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32 planNumber,
                                                  PlanWorkSpace  *pws);
private:

  //! PhysicalSPProxyFunc Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  PhysicalSPProxyFunc(const PhysicalSPProxyFunc &other);

}; // class PhysicalSPProxyFunc

// -----------------------------------------------------------------------
/*!
*  \brief ExtractSource Class.
*         The ExtractSource class represents a virtual table used on the
*         "consumer" side of a parallel extract operation. At runtime the
*         rows produced by this operator will be retrieved from an ESP server
*         that is working on a separate query known as the "producer"
*         query. This is a logical-only operator. The physical counterpart is
*         class PhysicalExtractSource, declared immediately below.
*/
// -----------------------------------------------------------------------
class ExtractSource : public ProxyFunc
{
public:
  // enums

  //! ExtractType enum 
  enum ExtractType { ESP = 1 };

  // constructors 
  //! ExtractSource Constructor 
  ExtractSource(ElemDDLNode *columnsFromParser,
                NAString &espPhandle,
                NAString &securityKey,
                CollHeap *heap);
  
  //! ExtractSource Constructor 
  // Default constructor
  ExtractSource(CollHeap *oHeap = CmpCommon::statementHeap());
  
  //! ExtractSource Copy Constructor 
  // Copy constructor without heap is not implemented nor allowed.
  ExtractSource(const ExtractSource &other, 
                CollHeap *h = CmpCommon::statementHeap());

  // destructors 

  //! ~ExtractSource Destructor 
  virtual ~ExtractSource();

  // acessors

  // The RelExpr interface

  //! getArity method 
  //  returns degree of the node
  virtual Int32 getArity() const { return 0; }

  //! isLogical method 
  //  method indicates if the node is a logical one
  virtual NABoolean isLogical() const { return TRUE; }

  //! isPhysical method 
  //  metod indicates if the node is a physical one
  virtual NABoolean isPhysical() const { return FALSE; }

  //! getText method 
  //  returns a constant NAString used to display name of the node
  virtual const NAString getText() const;

  //! topHash method 
  //  metod used by the optimizer to generate a hash for the node
  virtual HashValue topHash();

  //! isCacheableExpr method 
  //  metod to indicate if the node is cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa) { return FALSE; }

  //! getEspPhandle method 
  //  returns a reference to an NAString representing the espPhandle
  inline const NAString &getEspPhandle() const { return espPhandle_; }

  //! getSecurityKey method 
  //  returns a constant reference to an NAString representing the securityKey
  inline const NAString &getSecurityKey() const { return securityKey_; }

  // mutators

  // The RelExpr interface

  //! copyTopNode method 
  //  metod to create an Almost complete copy
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap* outHeap = 0);

  //! duplicateMatch method 
  //  metod used by the optimzier to determine identical nodes
  virtual NABoolean duplicateMatch(const RelExpr & other) const;


protected:
  //! espPhandle_ member 
  NAString espPhandle_;
  //! securityKey_ member 
  NAString securityKey_;

}; // class ExtractSource

// -----------------------------------------------------------------------
/*!
*  \brief PhysicalExtractSource Class.
*/
// -----------------------------------------------------------------------

class PhysicalExtractSource : public ExtractSource
{
public:
  // constructors


  //! PhysicalExtractSource Constructor 
  PhysicalExtractSource(CollHeap *oHeap = CmpCommon::statementHeap())
    : ExtractSource(oHeap)
  {}



  // The physical RelExpr interface
  // acessors 

  //! isLogical method
  //  method indicating if the node is logical
  virtual NABoolean isLogical() const { return FALSE; }

  //! isPhysical method 
  //  method indicating if the node is physical
  virtual NABoolean isPhysical() const { return TRUE; }
  
  // mutators

  //! codeGen method
  //  virtual method used by the generator to generate code for the node
  virtual short codeGen(Generator *);

  //! synthPhysicalProperty method
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32 planNumber,
                                                  PlanWorkSpace  *pws);
private:

  //! PhysicalExtractSource Copy Constructor 
  // Copy constructor without heap is not implemented nor allowed.
  PhysicalExtractSource(const ExtractSource &other);

}; // class PhysicalExtractSource

// -----------------------------------------------------------------------
/*!
*  \brief IsolatedNonTableUDR Class.
*         This is the superclass for CallSP and 
*         IsolatedScalarUDF class
*/
// -----------------------------------------------------------------------

class IsolatedNonTableUDR : public RelRoutine
{
public:
  // constructors

  
  //! IsolatedNonTableUDR Constructor
  //  expects a name and a heap
  IsolatedNonTableUDR ( QualifiedName routineName, CollHeap *outHeap,
                        OperatorTypeEnum spType = REL_ISOLATED_NON_TABLE_UDR ) :
               RelRoutine( routineName, NULL, spType, outHeap)
               {}

  //! IsolatedNonTableUDR Constructor
  //  expects a name, inParameter, outParameters and a heap
  IsolatedNonTableUDR ( QualifiedName routineName, ItemExprList *inParams, 
                        ValueIdList outParams, CollHeap *outHeap, 
                        OperatorTypeEnum spType = REL_ISOLATED_NON_TABLE_UDR ) :
               RelRoutine( routineName, outParams, 
                           inParams->convertToItemExpr(), spType, outHeap)
               {}



  // destructors


  //! ~IsolatedNonTableUDR Destructor
  virtual ~IsolatedNonTableUDR();


  // acessors

  //! getNeededValueIds
  //  returns a reference to the neededValueIds ValueIdSet
  inline ValueIdSet  & getNeededValueIds() { return neededValueIds_; }


  //! getInputExpressionName
  //  returns a constant char pointer to the input Expression name
  virtual const char *getInputExpressionName() const { return "input_values"; } 

  //! getText
  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // mutators


  //! setNeedeValueIds
  //  initializes the neededValueIds_ member to the give set
  inline void setNeededValueIds(ValueIdSet  neededValueIds) 
         { neededValueIds_ = neededValueIds; }


  //! populateAndBindItemExpr method
  // private methods used by the binder
  // Support for parameter mode processing
  // All these methods are in bindrelexpr.cpp
  virtual void populateAndBindItemExpr ( ItemExpr *param,
                                 BindWA *bindWA );


  //! setInOrOutParam method
  //  add the ItemExpr to the appropriate list of input and output params
  virtual void setInOrOutParam ( ItemExpr *expr,
                         ComColumnDirection mode,
                         BindWA *bindWA );

  //! copyTopNode method
  //  returns an Almost complete copy of the node
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL, 
                                 CollHeap* outHeap = 0);

  //! transformNode method
  // NormRelExpr.cpp
  virtual void transformNode (NormWA & normWARef,
                              ExprGroupId & locationOfPointerToMe);

  //! rewriteNode method 
  //  a virtual function for performing rewriting of expressions during 
  //  normalization phase.
  virtual void rewriteNode(NormWA &normWARef);

  //! preCodeGen method 
  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  //! addSpecificExplainInfo method
  //  generator method
  //  GenUdr.cpp

  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
                                       ComTdb * tdb,
                                       Generator *generator);


private:
   
  //! IsolatedNonTableUDR Constructor
  //  Default constructor without heap is not implemented nor allowed.
  IsolatedNonTableUDR (void);

  //! IsolatedNonTableUDR Copy Constructor
  //  Copy constructor without heap is not implemented nor allowed.
  IsolatedNonTableUDR( const IsolatedNonTableUDR & other);


  //! needeValueIds_ member
  //  This is used both by the transformer and normalizer.
  //  It is used in conjunction with the selectionPred to figure out
  //  what valueIds the node require. Since routine parameters are not
  //  regular predicates, we need to remeber them here.
  //  This set may be less than the procInputParamsVids list since ValueIdSets 
  //  eliminates duplicates. This is also the reason why we cannot use the
  //  procInputParamsVids list for both keeping track of routine input
  //  parameters and valueIds needed for pushdown/pullup processing.
  ValueIdSet          neededValueIds_;


}; // class IsolatedNonTableUDR


// -----------------------------------------------------------------------
/*!
*  \brief CallSP Class.
*         CallSP is the RelExpr for a CALL statement. It is both a logical
*         and physical operator.
*/
// -----------------------------------------------------------------------
class CallSP : public IsolatedNonTableUDR
{
public:

    // Convenience typedef
    typedef IsolatedNonTableUDR super;

    // constructors

    //! CallSP Constructor
    //  expects a name and a heap
    CallSP(const QualifiedName &routineName, CollHeap *h)
        : IsolatedNonTableUDR (routineName, h, REL_CALLSP),
          isPhysical_ (FALSE),
          duplicateVars_ (h),
          hasUDF_ (FALSE),
          outDupVars_ (h)
    {
    }

    // acessors


    //! getArity method
    // get the degree of this node. CallSP nodes generally are leaf nodes
    // except if we found a subquery or UDFs in the input parameters. In this
    // we create a tuple for that subquery and assign it as our child.
    // This holds true until we get to the optimizer. As soon as we set the
    // isPhysical_ flag to true, we no longer have a child.
    virtual Int32 getArity() const 
            { return isPhysical_ ? 0 : ((hasSubquery() || hasUDF()) ? 1 : 0) ; }

    //! getSPName method
    //  returns a reference to a constant name for the Routine
    inline const QualifiedName &getSPName ( void ) { return getRoutineName(); }


    //! isLogical method
    //  We are always a logical node
    virtual inline NABoolean isLogical() const { return TRUE; }

    //! isPhysical method
    //  We may or not be physical before optimization. We are definitely
    //  a physical node after optimization.
    virtual inline NABoolean isPhysical() const { return isPhysical_; }


    // RelExpr.cpp
    //! getText method
    //  gets constant NAString used to display the name of the node
    virtual const NAString getText() const;

    //! getPotentialOutputValues method
    //  returns a valueId set with the valueIds the node can produce for output
    virtual void getPotentialOutputValues(ValueIdSet &vs) const;

    //! hasUDF method
    //  Does this Routine have parameters containing subqueries?
    inline NABoolean hasUDF () const { return hasUDF_; }

    //! hasUDF method
    //  Does this Routine have parameters containing subqueries?
    inline NABoolean &hasUDF () { return hasUDF_; }

    // Mutators

    //! setPhysical method
    //  sets the isPhysical flag to True
    virtual inline void setPhysical () { isPhysical_ = TRUE; }

    //! setHasUDF method
    //  Does this Routine have parameters containing subqueries?
    inline void setHasUDF () { hasUDF_ = TRUE; }

    // RelExpr.cpp
    //! copyTopNode method
    //  returns an Almost complete copy of the node
    virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL, 
                                 CollHeap* outHeap = 0);

    //! bindNode method
    //  Binder code in BindRelExpr.cpp
    virtual RelExpr *bindNode(BindWA *bindWA);

    //! synthLogProp method
    //  Logical properties implemented in OptLogRelExpr.cpp
    virtual void synthLogProp(NormWA * normWAPtr = NULL);

    //! synthEstLogProp method
    //  Estimated Logical properties implemented in OptLogRelExpr.cpp
    virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

    //! synthPhysicalProp method
    //  Physical properties implemented in OptPhysRelExpr.cpp
    virtual PhysicalProperty *synthPhysicalProperty(const Context* myContext,
                                                    const Lng32     planNumber,
                                                    PlanWorkSpace  *pws);

    //! codeGen method
    //  virtual method in Generator code :generator/GenUdr.cpp
    virtual short codeGen(Generator *generator);


    //! pushdownCoveredExpr method
    // RelExpr.cpp
    virtual void pushdownCoveredExpr
                         (const ValueIdSet & outputExprOnOperator,
                          const ValueIdSet & newExternalInputs,
                          ValueIdSet & predicatesOnOperator,
                          const ValueIdSet * nonPredNonOutputExprOnOperator = NULL,
                          Lng32 childIndex = (-MAX_REL_ARITY));

    //! generateShape method
    // generator/GenShape.cpp, used by SHOWSHAPE <call-stmt>
    short generateShape(CollHeap * c, char * buf, NAString * string=NULL);

    //! generateCacheKey method
    // RelCache.cpp
    virtual void generateCacheKey(CacheWA &cwa) const;

    //! setArity method
    //  method to set how many children the node has
    void setArity ( Int32 numChildren )
    {
      CMPASSERT ((0 == numChildren || 1 == numChildren))
      RelRoutine::setArity(numChildren) ;
    }


private:

    //! CallSP Constructor
    // default constructor
    CallSP (void);


    //! setInOrOutParam method
    //  add the ItemExpr to the appropriate list of input and output params
    void setInOrOutParam ( ItemExpr *expr,
                           ComColumnDirection mode,
                           BindWA *bindWA );

    //! isPhysical_ member
    //  indicate if the node is a Physical one
    NABoolean           isPhysical_;

    //! hasUDF_ member
    //  indicate if the node has UDFs in its inputs
    NABoolean           hasUDF_;

    //! duplicateVars_ member
    // Check for duplicate variables, CLI support
    LIST(SPDupVarList *) duplicateVars_;

    //! outDupVars_ member
    // Check for duplicate OUT/INOUT variables, used to issue
    // a warning that output may be unpredictable
    LIST(SPDupVarList *) outDupVars_;


}; // class CallSP

// -----------------------------------------------------------------------
/*!
*  \brief IsolatedScalarUDF Class.
*         This class implements all Isolated Scalar UDFs 
*/
// -----------------------------------------------------------------------
class IsolatedScalarUDF : public IsolatedNonTableUDR
{
public:

    // Convenience typedef
    typedef IsolatedNonTableUDR super;

    // constructors

    //! IsolatedScalarUDF Constructor
    //  expects name and heap
    IsolatedScalarUDF(const QualifiedName &routineName, CollHeap *h)
        : IsolatedNonTableUDR (routineName, h, REL_ISOLATED_SCALAR_UDF)
    {
    }


    //! IsolatedScalarUDF Constructor
    //  expects name, input parameters, output parameters and  heap
    IsolatedScalarUDF(const QualifiedName &routineName, ItemExprList *inParams,
                            ValueIdList outParams, CollHeap *h)
        : IsolatedNonTableUDR (routineName, inParams, outParams, h,
                               REL_ISOLATED_SCALAR_UDF)
    {
    }

    // destructor

    //! ~IsolatedScalarUDF Destructor
    virtual ~IsolatedScalarUDF();

    // acessors

    //! getActionNARoutine method
    //  returns a pointer to an NARoutine struct for the action if we have one
    inline const NARoutine * getActionNARoutine() const
    {
      RoutineDesc *rdesc = getRoutineDesc();
      return rdesc == NULL ? NULL : rdesc->getActionNARoutine();
    }

    //! getProcOutParams method
    //  uses the outputs in the RoutineDesc to produce output for the class
    //  should work like this for all RelRoutine classes
    inline const ValueIdList & getProcOutParams() const
    {
      RoutineDesc *rdesc = getRoutineDesc();
      
      CMPASSERT (rdesc != NULL);
      return rdesc->getOutputColumnList();
    }


    //! isLogical method
    // We are always a logical node
    virtual inline NABoolean isLogical() const { return TRUE; }

    //! isPhysical method
    virtual inline NABoolean isPhysical() const { return FALSE; }

    //! getText method
    //  RelExpr interface. Returns NAString with the name of the node
    virtual const NAString getText() const;

    //! addLocalExpr method
    //  Add all the expressions that are local to this node to an
    //  existing list of expressions (used by GUI tool). addLocalExpr()
    //  will call the virtual getInputExpressionName() method to
    //  determine the name of the input expression.
    virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                              LIST(NAString) &llist) const;

    //! getPotentialOutputValues method
    //  RelExpr interface. Returns a valueIdSet of all outputs the node 
    //  can produce
    virtual void getPotentialOutputValues(ValueIdSet &vs) const;


    // Mutators

    // RelExpr.cpp
    //! copyTopNode method
    //  returns a pointer to an Almost complete copy of the node
    virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL, 
                                 CollHeap* outHeap = 0);


    //! bindNode method
    // Binder code in BindRelExpr.cpp
    virtual RelExpr *bindNode(BindWA *bindWA);

    //! synthLogProp method
    // Logical properties implemented in OptLogRelExpr.cpp
    virtual void synthLogProp(NormWA * normWAPtr = NULL);

    //! synthEstLogProp method
    // Logical properties implemented in OptLogRelExpr.cpp
    virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

    //! synthPysicalProperty method
    // Logical properties implemented in OptLogRelExpr.cpp
    virtual PhysicalProperty *synthPhysicalProperty(const Context* myContext,
                                                    const Lng32     planNumber,
                                                    PlanWorkSpace  *pws);

    //! costMethod method
    //  cost functions
    virtual CostMethod *costMethod() const;

    //! codeGen method
    // Generator code in generator/GenUdr.cpp
    virtual short codeGen(Generator *generator);


    //! generateShape method
    // generator/GenShape.cpp, used by SHOWSHAPE <call-stmt>
    virtual short generateShape(CollHeap * c, char * buf, NAString * string=NULL);

    //! generateCacheKey method
    // RelCache.cpp
    virtual void generateCacheKey(CacheWA &cwa) const;



private:

    //! IsolatedScalarUDF Constructor
    //  Default constructor without heap is not implemented nor allowed.
    IsolatedScalarUDF (void);

    //! IsolatedScalarUDF Copy Constructor
    //  Copy constructor without heap is not implemented nor allowed.
    IsolatedScalarUDF( const IsolatedScalarUDF & other);


}; // class IsolatedScalarUDF

// -----------------------------------------------------------------------
/*!
*  \brief PhysicalIsolatedScalarUDF Class.
*         The PhysicalIsolatedScalarUDF replaces the logical IsolatedScalarUDF 
*         through the application of the PhysicalIsolatedScalarUDFRule. 
*         This transformation is designed to present a physical verison 
*         of an operator. In our case a Logical IsolatedScalarUDF has only
*         one physical representation, so we could have done without it 
*         but the decree is that each logical operator SHOULD have one.
*/
// -----------------------------------------------------------------------
class PhysicalIsolatedScalarUDF : public IsolatedScalarUDF
{
public:
  // constructors 

  //! PhysicalIsolatedScalarUDF Constructor
  PhysicalIsolatedScalarUDF(CollHeap *);

  // acessors 

  //! isLogical method
  //  indicate if the node is a logical node
  virtual NABoolean isLogical() const { return FALSE ; }

  //! isPhysical method
  //  indicate if the node is a physical node
  virtual NABoolean isPhysical() const { return TRUE; } 

  //! costMethod method
  //  cost functions
  virtual CostMethod *costMethod() const;

  virtual NABoolean patternMatch(const RelExpr & other) const;


private:
  // constructors 


  //! PhysicalIsolatedScalarUDF Copy Constructor
  //  Copy constructor without heap is not implemented nor allowed.
  PhysicalIsolatedScalarUDF(const PhysicalIsolatedScalarUDF &);

}; // class PhysicalIsolatedScalarUDF


// -----------------------------------------------------------------------
/*!
*  \brief SPDupVarList Class.
*         List used to check if there are duplicate Host Vars/Dynamic params
*         Used to report warnings if a HV is used as multiple OUT/INOUT 
*         parameters.
*         Used by Stored Procedures (for Java)
*/
// -----------------------------------------------------------------------
class SPDupVarList : public NABasicObject
{
    friend class CallSP;
public:
    // constructors

   //! SPDupVarList Constructor
    SPDupVarList ( const NAString &name,
                  ComColumnDirection direction,
                  Int32 varIndex ):
        name_ (name),
        direction_ (direction),
        varIndex_ (varIndex)
    {
    }

#if 0
    ~SPDupVarList ()
    {
      printf ( "Destructor called for dupVarCheck\n" );
    }
#endif

private:
    //! SPDupVarList Default Constructor
    //  Default constructor without heap is not implemented nor allowed.
    SPDupVarList (void);

    //! SPDupVarList Copy Constructor
    //  Copy constructor without heap is not implemented nor allowed.
    SPDupVarList (SPDupVarList &other);

   //! name_ member
    const NAString      &name_;

   //! direction_ member
    ComColumnDirection  direction_;

   //! varIndex_ member
    Int32                 varIndex_;
};

#endif /* RELROUTINE_H */
