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
#ifndef RELFASTTRANSPORT_H
#define RELFASTTRANSPORT_H
/* -*-C++-*-
**************************************************************************
*
* File:         RelFastTransport.h
* Description:  RelExprs related to support of FastTransport
* Created:      09/29/12
* Language:     C++
*
*************************************************************************
*/


#include "RelExpr.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class FastExtract ;
class PhysicalFastExtract;

  class UnloadOption
  {
    friend class FastExtract;
    friend class ExeUtilHBaseBulkUnLoad;
  public:
    enum UnloadOptionType {
      DELIMITER_,
      NULL_STRING_,
      RECORD_SEP_,
      APPEND_,
      HEADER_,
      COMPRESSION_,
      EMPTY_TARGET_,
      LOG_ERRORS_,
      STOP_AFTER_N_ERRORS_,
      NO_OUTPUT_,
      COMPRESS_,
      ONE_FILE_,
      USE_SNAPSHOT_SCAN_
    };
    UnloadOption(UnloadOptionType option, Lng32 numericVal, char * stringVal, char * stringVal2 = NULL )
    : option_(option), numericVal_(numericVal), stringVal_(stringVal)
    {};
  private:
    UnloadOptionType option_;
    Lng32   numericVal_;
    char * stringVal_;
  };

// -----------------------------------------------------------------------
/*!
*  \brief FastExtract Class.
*        FastExtract is a RelExpr that takes the rows produced by its child,
*        formats them to a character format, writes the result into either
*        flat files or tcp/ip sockets
*/
// -----------------------------------------------------------------------

class FastExtract: public RelExpr
{
public :

  enum ExtractDest {INVALID = 0, FILE=1, SOCKET=2};
  enum CompressionType {NONE=0, LZO=1};

  // constructors

  //! FastExtract Constructor
  FastExtract(RelExpr* child,
      NAString* targName,
      NAString* delim ,
      NAString* nullString ,
      NAString* recordSep ,
      ExtractDest targType = FILE,
      NABoolean isAppend = FALSE,
      NABoolean needsHeader = TRUE,
      CompressionType cType= NONE,
      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(REL_FAST_EXTRACT, child, NULL, oHeap),
    targetType_(targType),
    targetName_(*targName, oHeap),
    hdfsHostName_(oHeap),
    hdfsPort_(0),
    hiveTableDesc_(NULL),
    delimiter_(*delim, oHeap),
    isAppend_(isAppend),
    includeHeader_(needsHeader),
    header_(oHeap),
    cType_(cType),
    nullString_(*nullString, oHeap),
    recordSeparator_(*recordSep, oHeap),
    overwriteHiveTable_(FALSE),
    isSequenceFile_(FALSE),
    nullStringSpec_((nullString ? TRUE : FALSE)),
    isMainQueryOperator_(TRUE)
  {
  };

  FastExtract(RelExpr* child,
      NAString* targName,
      ExtractDest targType,
      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(REL_FAST_EXTRACT, child, NULL, oHeap),
    targetType_(targType),
    targetName_(*targName, oHeap),
    hdfsHostName_(oHeap),
    hdfsPort_(0),
    hiveTableDesc_(NULL),
    delimiter_(oHeap),
    isAppend_(FALSE),
    includeHeader_(FALSE),
    header_(oHeap),
    cType_(NONE),
    nullString_(oHeap),
    recordSeparator_(oHeap),
    overwriteHiveTable_(FALSE),
    isSequenceFile_(FALSE),
    nullStringSpec_(FALSE),
    isMainQueryOperator_(TRUE)
  { };


  FastExtract(RelExpr* child,
      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(REL_FAST_EXTRACT, child, NULL, oHeap),
    targetType_(INVALID),
    targetName_(oHeap),
    hdfsHostName_(oHeap),
    hdfsPort_(0),
    hiveTableDesc_(NULL),
    delimiter_(oHeap),
    isAppend_(FALSE),
    includeHeader_(FALSE),
    header_(oHeap),
    cType_(NONE),
    nullString_(oHeap),
    recordSeparator_(oHeap),
    overwriteHiveTable_(FALSE),
    isSequenceFile_(FALSE),
    nullStringSpec_(FALSE),
    isMainQueryOperator_(TRUE)
  { };

  FastExtract(RelExpr* child,
      NAString* targName,
      NAString* hostName,
      Int32 portNum,
      TableDesc *hiveTableDesc,
      NAString* hiveTableName,
      ExtractDest targType,
      CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(REL_FAST_EXTRACT, child, NULL, oHeap),
    targetType_(targType),
    targetName_(*targName, oHeap),
    hdfsHostName_(*hostName, oHeap),
    hdfsPort_(portNum),
    hiveTableDesc_(hiveTableDesc),
    hiveTableName_(*hiveTableName, oHeap),
    delimiter_(oHeap),
    isAppend_(FALSE),
    includeHeader_(FALSE),
    header_(oHeap),
    cType_(NONE),
    nullString_(oHeap),
    recordSeparator_(oHeap),
    overwriteHiveTable_(FALSE),
    isSequenceFile_(FALSE),
    nullStringSpec_(FALSE),
    isMainQueryOperator_(TRUE)
  { };

  //! FastExtract Copy Constructor
  FastExtract(const FastExtract &other);

  //! ~FastExtract destructor
  virtual ~FastExtract();

  // make a FastExtract from a TableDesc
  static RelExpr * makeFastExtractTree(
       TableDesc *tableDesc,
       RelExpr *child,
       NABoolean overwriteTable,
       NABoolean calledFromBinder,
       NABoolean tempTableForCSE,
       BindWA *bindWA);

  //! copyTopNode method
  // a virtual function used to copy most of a Node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! getText method
  // a virtual function for displaying the name of the node
  virtual const NAString getText() const ;

  virtual RelExpr* bindNode(BindWA* bindWA); 
  virtual void transformNode(NormWA & normWARef,
    ExprGroupId & locationOfPointerToMe);
  virtual void rewriteNode(NormWA & normWARef) ;
  virtual RelExpr * normalizeNode(NormWA & normWARef);
  void generateCacheKey(CacheWA &cwa) const;
 

  // --------------------------------------------------------------------
  // A method that provides the set of values that can potentially be
  // produced as output by this operator. These are values over and
  // above those that are produced as Characteristic Outputs.
  // --------------------------------------------------------------------
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;
  virtual void pullUpPreds();

  virtual void pushdownCoveredExpr(
                                   const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet & newExternalInputs,
                                   ValueIdSet& predOnOperator,
                                   const ValueIdSet *
                                   	   nonPredNonOutputExprOnOperator = NULL,
                                   Lng32 childId = (-MAX_REL_ARITY));

  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputLP) ;

  // ---------------------------------------------------------------------
  // comparison, hash, and copy methods
  // ---------------------------------------------------------------------

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  // --------------------------------------------------------------------
  // Methods used internally by Cascades
  // --------------------------------------------------------------------

  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const {return TRUE;}
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const {return FALSE;}


  // ---------------------------------------------------------------------
  // for debugging
  // ---------------------------------------------------------------------

  // add all the expressions that are local to this
  // node to an existing list of expressions
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;


  virtual NABoolean isBigMemoryOperator(const Context* context,
                                        const Lng32 /*planNumber*/)
  {return FALSE;}



  virtual Int32 getArity() const {return 1 ;}

  short setOptions(NAList<UnloadOption*> * feol, ComDiagsArea * da);
  const ValueIdList & getSelectList() const	      {return selectList_; }
  ValueIdList & getSelectListIds() {return selectList_; }
  void setSelectList(const ValueIdList & val)    {selectList_ = val; }
  ExtractDest getTargetType() const {return targetType_; }
  const NAString& getTargetName() const {return targetName_;}
  const NAString& getHdfsHostName() const {return hdfsHostName_;}
  Int32 getHdfsPort() const {return hdfsPort_;}
  const NAString& getHiveTableName() const {return hiveTableName_;}
  NABoolean isHiveInsert() const {return (hiveTableDesc_ != NULL);}
  const TableDesc* getHiveTableDesc() const { return hiveTableDesc_; }
  const NAString& getDelimiter() const {return delimiter_;}
  NABoolean isAppend() const {return isAppend_;}
  NABoolean includeHeader() const {return includeHeader_ ;}
  const NAString& getHeader() const {return header_;}
  CompressionType getCompressionType() const {return cType_ ; }
  const NAString& getNullString() const {return nullString_ ;}
  const NAString& getRecordSeparator()  const {return recordSeparator_ ; }
  void setRecordSeparator(char * v)
  {
    recordSeparator_ = v;
  }
  void setDelimiter(char * v )
  {
    delimiter_ = v;
  }
  
  NABoolean getOverwriteHiveTable() const
  {
   return overwriteHiveTable_;
  }
  
  void setOverwriteHiveTable(NABoolean overwriteHiveTable)
  {
   overwriteHiveTable_ = overwriteHiveTable;
  }

  NABoolean isSequenceFile() const
  {
   return isSequenceFile_;
  }
  
  void setSequenceFile(NABoolean sf)
  {
   isSequenceFile_ = sf;
  }

  void setIsMainQueryOperator(NABoolean m)
  {
    isMainQueryOperator_ = m;
  }

  NABoolean getIsMainQueryOperator() const
  {
    return isMainQueryOperator_;
  }

private:
  
  
  ExtractDest targetType_ ;
  NAString targetName_; 
  NAString hdfsHostName_ ; // to be used only for hive inserts
  Int32 hdfsPort_; // to be used only for hive inserts
  ValueIdList selectList_;
  NAString delimiter_;
  NABoolean includeHeader_;
  NAString header_;
  CompressionType cType_;
  NAString nullString_;
  NABoolean nullStringSpec_; // if null format string is specified
  NAString recordSeparator_;
  NABoolean isAppend_;
  TableDesc *hiveTableDesc_;
  NAString hiveTableName_;
  NABoolean overwriteHiveTable_;
  NABoolean isSequenceFile_;
  NABoolean isMainQueryOperator_;

}; // class FastExtract

// -----------------------------------------------------------------------
/*!
*  \brief PhysicalFastExtract Class.
*         The PhysicalFastExtract replaces the logical FastExtract
*         through the application of the PhysicalFastExtractRule.
*         This transformation is designed to present a purely
*         physical verison of an operator
*         that is both logical and physical.
*/
// -----------------------------------------------------------------------
class PhysicalFastExtract : public FastExtract
{
public:
  // constructor

  //! PhysicalFastExtract Constructor
  PhysicalFastExtract(RelExpr * child,
		  	  	  	  CollHeap *oHeap = CmpCommon::statementHeap())
    : FastExtract(child, oHeap)
  {};

  //! isLogical method
  //  indicate if the node is logical
  virtual NABoolean isLogical() const { return FALSE; }
  //! isPhysical method
  //  indicate if the node is physical
  virtual NABoolean isPhysical() const { return TRUE; }

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

  virtual double getEstimatedRunTimeMemoryUsage(Generator *generator, ComTdb * tdb) ;

  virtual short codeGen(Generator *);
  static NABoolean isSpecialChar(char * str , char & chr);

  
  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple, ComTdb * tdb, Generator *generator);


}; // class PhysicalFastExtract



#endif /* RELFASTTRANSPORT_H */
