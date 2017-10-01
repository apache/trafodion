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
#ifndef REFRESH_H
#define REFRESH_H

/* -*-C++-*-
******************************************************************************
*
* File:         Refresh.h
* Description:  Definition of class Refresh for INTERNAL REFRESH command.
*               Used for refreshing Materialized Views.
*
* Created:      12/19/99
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


#include "ComMvDefs.h"
#include "RelMisc.h"

// classes defined in this file
class Refresh;
class DeltaOptions;
class DeltaDefinition;
class PipelineClause;
class DeltaDefinitionPtrList;
class DeltaDefLogs;
class DeltaDefRangeLog;
class DeltaDefIUDLog;
class PipelineDef;
class PipelineDefPtrList;
class RecomputeRefreshOption;
class NRowsClause;
class IncrementalRefreshOption;
class IUDStatistics;

// Forward references 
class QualifiedName;


// -----------------------------------------------------------------------
// The Refresh operator is used by the INTERNAL REFRESH command for 
// materialized views.refreshing
// -----------------------------------------------------------------------
class Refresh : public BinderOnlyNode
{
public:
  enum RefreshType { RECOMPUTE , SINGLEDELTA, MULTIDELTA };
  
  // constructors, destructor

  // Ctor for RECOMPUTE 
  Refresh(const QualifiedName& mvName, 
	  ComBoolean           noDelete,
	  CollHeap            *oHeap = CmpCommon::statementHeap());
  
  // Ctor for SINGLEDELTA
  Refresh(const QualifiedName&          mvName, 
	  const DeltaDefinitionPtrList *pDeltaDefList,
	  NRowsClause                  *pOptionalNRowsClause,
	  PipelineClause               *pOptionalPipelineClause,
	  CollHeap                     *oHeap = CmpCommon::statementHeap());

  // Ctor for MULTIDELTA
  Refresh(const QualifiedName&          mvName, 
	  const DeltaDefinitionPtrList *pDeltaDefList,
	  Lng32	                        phaseVal,
	  PipelineClause               *pOptionalPipelineClause,
	  CollHeap                     *oHeap = CmpCommon::statementHeap());

  virtual ~Refresh();

  virtual void cleanupBeforeSelfDestruct();

  // Accessors
  RefreshType GetRefreshType() const { return refreshType_; }
  const QualifiedName& getBaseMvName() { return mvName_; }
  const PipelineClause *getPipelineClause() const { return pipelineClause_; }
  const DeltaDefinitionPtrList *getDeltaDefList() const { return deltaDefList_; }
  Lng32  getPhase() const { return phase_; }
  const NRowsClause *getNRowsClause() const { return pNRowsClause_; }
  NABoolean getAdditionalPhaseNeeded() const { return additionalPhaseNeeded_; }
  DeltaDefinition *getDeltaDefinitionFor(const QualifiedName& name);

  // Mutators
  void setAdditionalPhaseNeeded() 
    { additionalPhaseNeeded_ = TRUE; }

  // Other methods

  // Get the MV SELECT text from the MVInfo and parse it.
  RelRoot *getMVSelectTree(BindWA *bindWA, NATable *mavNaTable, MVInfo *&mvInfo);
  // return the MV that fromMV is pipelining to.
  const QualifiedName *getNextLevelMv(const QualifiedName& fromMV) const;
  // Returns TRUE if we should and can use the specific MinMax builder.
  NABoolean canUseMinMaxBuilder(BindWA *bindWA, MVInfoForDML *mvInfo);
  NABoolean doesBaseTableHaveSupportingIndex(BindWA *bindWA, MVInfoForDML *mvInfo) const;
  NABoolean areGroupByColsAnIndexPrefix(const NAColumnArray & groupByCols,
				        const NAColumnArray & columnArray) const;

  // This is where all the action begins.
  virtual RelExpr *bindNode(BindWA *bindWAPtr);
  // Determine the correct MvRefreshBuilder sub-class to handle the work,
  MvRefreshBuilder *constructRefreshBuilder(BindWA *bindWA, MVInfoForDML *mvInfo);
  MvRefreshBuilder *constructMavSpecifichBuilder(BindWA       *bindWA,
						 CorrName      mvCorrName,
						 MVInfoForDML *mvInfo);
  MvRefreshBuilder *constructMjvSpecifichBuilder(BindWA       *bindWA,
						 CorrName      mvCorrName,
						 MVInfoForDML *mvInfo);
  RelExpr *constructPipelinedBuilders(BindWA        *bindWA, 
				      MvBindContext *bindContext,
				      RelExpr       *firstRefreshTree);

  // Build the all the parts of the refresh tree. These parts will be
  // put together during the binding.
  RelExpr *buildCompleteRefreshTree(BindWA *bindWA, MVInfoForDML *mvInfo);

  virtual RelExpr *copyTopNode(RelExpr  *derivedNode = NULL,
			       CollHeap *outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

private:
  MVInfoForDML *getMvInfo(BindWA *bindWA, CorrName& mvToRefresh) const;
  NABoolean verifyDeltaDefinition(BindWA *bindWA, MVInfoForDML *mvInfo) const;

  // Data initialized in the Ctor (by the parser).
  const RefreshType		refreshType_;
        QualifiedName		mvName_;
  const DeltaDefinitionPtrList *deltaDefList_;
  const Lng32			phase_;
  PipelineClause	       *pipelineClause_;
  NRowsClause                  *pNRowsClause_;
  NABoolean			noDeleteOnRecompute_;

  // Data used during binding.
  MvBindContext                *bindContext_;
  // In Multi-Delta refresh - Do we need to return a warning that we are not 
  // done yet.
  NABoolean			additionalPhaseNeeded_;
}; // class Refresh

//----------------------------------------------------------------------------
// HELPER CLASSES 
// These classes are used by the parser to build the parameters for the 
// Internal Refresh command. 
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
class IUDStatistics : public NABasicObject
{
public:
  IUDStatistics(Lng32	      numInsertedRows,
		Lng32	      numDeletedRows,
		Lng32	      numUpdatedRows,
		IntegerList  *optionalColumns) 
  : numInsertedRows_(numInsertedRows), 
    numDeletedRows_(numDeletedRows),
    numUpdatedRows_(numUpdatedRows),
    optionalColumns_(optionalColumns)
  {}

  // Note: in destruction we do not delete the optionalColumns_ this 
  // will be done by DeltaDefinition
  virtual ~IUDStatistics() {}

  Lng32         getNumRowsInserted()    { return numInsertedRows_; }
  Lng32         getNumRowsDeleted()     { return numDeletedRows_; }
  Lng32         getNumRowsUpdated()     { return numUpdatedRows_; }
  IntegerList *getOptionalColumnList() { return optionalColumns_; }

private:
  // Copy Ctor and = operator are not implemented.
  IUDStatistics(const IUDStatistics& other);
  IUDStatistics& operator=(const IUDStatistics& other);

  Lng32         numInsertedRows_;
  Lng32         numDeletedRows_;
  Lng32         numUpdatedRows_;
  IntegerList *optionalColumns_;
};  // class IUDStatistics

//----------------------------------------------------------------------------
class DeltaDefIUDLog : public NABasicObject
{
public: 
  enum Option {NO_LOG, INSERT_ONLY, STAT, NO_STAT };

  DeltaDefIUDLog(Option option, IUDStatistics * optionalStat = NULL) 
  : op_(option), 
    optionalStat_(optionalStat) 
  {}

  virtual ~DeltaDefIUDLog()
  {
    delete optionalStat_; 
  }

  IUDStatistics *getStatistics() { return optionalStat_;}
  Option         getOption()     { return op_; }

private:
  // Copy Ctor and = operator are not implemented.
  DeltaDefIUDLog(const DeltaDefIUDLog& other);
  DeltaDefIUDLog& operator=(const DeltaDefIUDLog& other);

  Option         op_;
  IUDStatistics *optionalStat_; 
};  // class DeltaDefIUDLog

//----------------------------------------------------------------------------
class DeltaDefRangeLog : public NABasicObject
{
public:
  enum Option {NO_LOG, CARDINALITY_ONLY, ALL };

  DeltaDefRangeLog(Option option, Lng32 numOfRanges = 0, Lng32 coveredRows = 0) 
    : op_(option), 
      numOfRanges_(numOfRanges),
      coveredRows_(coveredRows)
  {}

  virtual ~DeltaDefRangeLog() {}

  Lng32   getNumOfRanges() { return numOfRanges_;}
  Lng32   getCoveredRows() { return coveredRows_;}

  Option getOption()       { return op_; }

private:
  // Copy Ctor and = operator are not implemented.
  DeltaDefRangeLog(const DeltaDefRangeLog& other);
  DeltaDefRangeLog& operator=(const DeltaDefRangeLog& other);

  Option op_;
  Lng32   numOfRanges_; 
  Lng32   coveredRows_;
};  // class DeltaDefRangeLog

//----------------------------------------------------------------------------
class DeltaDefLogs : public NABasicObject
{
public: 
  DeltaDefLogs(DeltaDefRangeLog * rangeDef, DeltaDefIUDLog * iudDef)
  : rangeDef_(rangeDef), 
    iudDef_(iudDef) 
  {}

  virtual ~DeltaDefLogs()
  {
    delete rangeDef_;
    delete iudDef_;
  }

  DeltaDefRangeLog* getRangeDef() { return rangeDef_; }
  DeltaDefIUDLog*  getIUDDef()    { return iudDef_; }

private:
  // Copy Ctor and = operator are not implemented.
  DeltaDefLogs(const DeltaDefLogs& other);
  DeltaDefLogs& operator=(const DeltaDefLogs& other);

  DeltaDefRangeLog * rangeDef_;
  DeltaDefIUDLog * iudDef_;
};  // class DeltaDefLogs

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
class DeltaOptions : public NABasicObject
{
public:
  enum DELevel { NO_DE, 
		 RANGE_RESOLUTION_ONLY, 
		 RANGE_AND_CROSS_TYPE_RESOLUTIONS, 
		 ALL};

  DeltaOptions(Lng32 deLevel, DeltaDefLogs *pDeltaDefLogs)
  : deLevel_(static_cast<DELevel>(deLevel)),
    pDeltaDefLogs_(pDeltaDefLogs)
  {}

  virtual ~DeltaOptions()
  {
    if (pDeltaDefLogs_) delete pDeltaDefLogs_;
  }

  DELevel	   getDELevel() const { return deLevel_;}

  NABoolean        getIsInsertOnly() 
  { 
    return pDeltaDefLogs_->getIUDDef()->getOption() ==  
	   DeltaDefIUDLog::INSERT_ONLY;
  }

  DeltaDefLogs	  *getDeltaStats()   { return pDeltaDefLogs_;}

private:
  // Copy Ctor and = operator are not implemented.
  DeltaOptions(const DeltaOptions& other);
  DeltaOptions& operator=(const DeltaOptions& other);

  const DELevel deLevel_;
  DeltaDefLogs    *pDeltaDefLogs_;
};

//----------------------------------------------------------------------------
//  The DeltaDefinition class holds the details of a specific log used by
//  the Refresh command. Objects of this class are constructed by the parser,
//  and used by the bindNode() method of the Refresh RelExpr class.
//----------------------------------------------------------------------------
class DeltaDefinition : public NABasicObject
{
public:
  DeltaDefinition(QualifiedName  *tableName,
		  Lng32		  beginEpoch,
		  Lng32		  endEpoch,
		  DeltaOptions	 *pDeltaOptions) 
  : tableName_(tableName),
    beginEpoch_(beginEpoch),
    endEpoch_(endEpoch),
    useIudLog_(TRUE),
    useRangeLog_(FALSE),
    insertOnly_(FALSE), 
    numOfRanges_(0),
    coveredRows_(0),
    iudInsertedRows_(1),
    iudDeletedRows_(1),
    iudUpdatedRows_(1),
    updatedColumnList_(NULL),
    pDeltaOptions_(pDeltaOptions),
    updateColumnsProcessed_(FALSE),
    containsUpdateColUsedByMv_(FALSE),
    containsIndirectUpdateColumn_(FALSE),
    containsDirectUpdateColumn_(FALSE)
  {}
  
  // This is a shortcut Ctor for a NO DE delta on a table.
  // It is used by PipelinedMavBuilder for faking the "delta definition"
  // of the pipelining MAV.
  DeltaDefinition(QualifiedName *tableName)
  : tableName_(tableName),
    beginEpoch_(0),
    endEpoch_(0),
    useIudLog_(TRUE),
    useRangeLog_(FALSE),
    insertOnly_(FALSE), 
    numOfRanges_(0),
    coveredRows_(0),
    iudInsertedRows_(1),
    iudDeletedRows_(1),
    iudUpdatedRows_(1),
    updatedColumnList_(NULL),
    pDeltaOptions_(NULL),
    updateColumnsProcessed_(FALSE),
    containsUpdateColUsedByMv_(FALSE),
    containsIndirectUpdateColumn_(FALSE),
    containsDirectUpdateColumn_(FALSE)
  {}

  virtual ~DeltaDefinition();
  
  // Properly initialize internal data structures.
  void synthesize();

  // Accessors
  QualifiedName	    *getTableName()         const { return tableName_; }
  Lng32	             getBeginEpoch()        const { return beginEpoch_; }
  Lng32	             getEndEpoch()          const { return endEpoch_; }
  NABoolean	     useIudLog()            const { return useIudLog_; }
  NABoolean	     useRangeLog()          const { return useRangeLog_; }
  
  NABoolean	     isNoDE()               const; 

  NABoolean	     isNoDeInsertOnly()     const { return insertOnly_; }
  Lng32		     getNumOfRanges()	    const { return numOfRanges_; }
  Lng32		     getCoveredRows()	    const { return coveredRows_; }

  Lng32	             getIudInsertedRows()   const { return iudInsertedRows_; }
  Lng32	             getIudDeletedRows()    const { return useIudLog() ? iudDeletedRows_ : 0; }
  Lng32	             getIudUpdatedRows()    const { return useIudLog() ? iudUpdatedRows_ : 0; }
  const IntegerList *getUpdatedColumnList() const { return updatedColumnList_; }
  DeltaOptions::DELevel getDELevel() const;
  NABoolean isFullDE() const { return getDELevel() == DeltaOptions::ALL; }

  NABoolean isInsertOnly() const 
  {
    if (isNoDE())
      return isNoDeInsertOnly();
    else 
      return (getIudDeletedRows() == 0) && 
             (getIudUpdatedRows() == 0);
  }

  NABoolean isDeleteOnly() const 
  {
    return !isNoDE()                   &&
           !useRangeLog()              &&
           (getIudInsertedRows() == 0) && 
           (getIudUpdatedRows()  == 0);
  }

  NABoolean updateColumnsNotUsedByMV(MVInfoForDML *mvInfo);

  NABoolean containsDirectUpdateColumn(MVInfoForDML *mvInfo);

  NABoolean containsIndirectUpdateColumn(MVInfoForDML *mvInfo);
  
  void processUpdateColumns(MVInfoForDML *mvInfo);

private:
  // Copy Ctor and = operator are not implemented.
  DeltaDefinition(const DeltaDefinition& other);
  DeltaDefinition& operator=(const DeltaDefinition& other);

  QualifiedName *tableName_;
  const Lng32	 beginEpoch_;
  const Lng32	 endEpoch_;
  NABoolean	 useIudLog_;
  NABoolean	 useRangeLog_;
  NABoolean	 insertOnly_;
  Lng32		 numOfRanges_;
  Lng32		 coveredRows_;
  Lng32		 iudInsertedRows_;
  Lng32		 iudDeletedRows_;
  Lng32		 iudUpdatedRows_;
  IntegerList   *updatedColumnList_;
  DeltaOptions  *pDeltaOptions_;
  NABoolean     updateColumnsProcessed_;
  NABoolean     containsUpdateColUsedByMv_;
  NABoolean     containsIndirectUpdateColumn_;
  NABoolean     containsDirectUpdateColumn_;
}; // class DeltaDefinition

//----------------------------------------------------------------------------
// This class holds all the information on pipelined refresh of MVs.
// It is constructed by the parser, and used by Refresh::bindNode().
// firstPipelineDef_ is the first set of MVs that use the base MV (that
//  is a direct parameter to Refresh(), and not part of the PipelineClause.
// pipelineDefList_ is an optional list of additional pipeline clauses.
//----------------------------------------------------------------------------
class PipelineClause : public NABasicObject
{
public:	
  PipelineClause(QualNamePtrList *firstPipelineDef, 
		 PipelineDefPtrList *defList = NULL)
 : firstPipelineDef_(firstPipelineDef),
   pipelineDefList_(defList)
 {}

  virtual ~PipelineClause();

  const QualNamePtrList *getFirstPipelineDef() const 
  { return firstPipelineDef_; }

  // return the MV that fromMV is pipelining to.
  const QualifiedName *getNextLevelMv(const QualifiedName& fromMV) const;

  // Apply the default catalog and schema to all the MV names.
  void applyDefaultSchemaToAll(BindWA *bindWA);

private:
  // Copy Ctor and = operator are not implemented.
  PipelineClause(const PipelineClause& other);
  PipelineClause& operator=(const PipelineClause& other);

  const QualNamePtrList    *firstPipelineDef_;
  const PipelineDefPtrList *pipelineDefList_;
};  // class PipelineClause

//----------------------------------------------------------------------------
class DeltaDefinitionPtrList : public LIST(DeltaDefinition *)
{
public:
  DeltaDefinitionPtrList(CollHeap* h = CmpCommon::statementHeap()) 
  : LIST(DeltaDefinition *)(h) 
  {}

  virtual ~DeltaDefinitionPtrList()
  {
    DeltaDefinition *deltaDef;
    while (getLast(deltaDef))
      delete deltaDef;
  }

  void  applyDefaultSchemaToAll(BindWA *bindWA) const;
  DeltaDefinition *findEntryFor(const QualifiedName& tableName) const;

  NABoolean areAllDeltasInsertOnly() const;

  NABoolean areAllDeltasWithFullDE() const;

private:
  // Copy Ctor and = operator are not implemented.
  DeltaDefinitionPtrList(const DeltaDefinitionPtrList& other);
  DeltaDefinitionPtrList& operator=(const DeltaDefinitionPtrList& other);
};  // class DeltaDefinitionPtrList

//----------------------------------------------------------------------------
class PipelineDef : public NABasicObject
{
public:
  PipelineDef(QualifiedName *fromClause, QualNamePtrList *toClause) 
  : fromClause_(fromClause),
    toClause_(toClause)
  {}

  virtual ~PipelineDef()
  {
    delete fromClause_;
    delete toClause_;
  }

  QualifiedName	  *getFromClause() const { return fromClause_; }
  QualNamePtrList *getToClause()   const { return toClause_; }

private:
  // Copy Ctor and = operator are not implemented.
  PipelineDef(const PipelineDef& other);
  PipelineDef& operator=(const PipelineDef& other);

  QualifiedName	  *fromClause_;
  QualNamePtrList *toClause_;
};  // class PipelineDef

//----------------------------------------------------------------------------
class PipelineDefPtrList : public LIST(PipelineDef *)
{
public:
  PipelineDefPtrList(CollHeap* h=CmpCommon::statementHeap()) 
  : LIST(PipelineDef *)(h) 
  {}

  virtual ~PipelineDefPtrList()
  {
    PipelineDef *def;
    while (getLast(def))
      delete def;
  }

private:
  // Copy Ctor and = operator are not implemented.
  PipelineDefPtrList(const PipelineDefPtrList& other);
  PipelineDefPtrList& operator=(const PipelineDefPtrList& other);
}; // class PipelineDefPtrList



//----------------------------------------------------------------------------
class RecomputeRefreshOption : public NABasicObject
{
public:
  RecomputeRefreshOption(NABoolean	      noDelete = FALSE,
	  		 const Lng32	      mvUidMsbForRecompute = 0,
			 const Lng32	      mvUidLsbForRecompute = 0,
			 const QualifiedName *tableName = NULL)
  : noDelete_(noDelete) 
  {}

  virtual ~RecomputeRefreshOption() {}

  NABoolean getIsNoDelete() { return noDelete_;}

private:
  // Copy Ctor and = operator are not implemented.
  RecomputeRefreshOption(const RecomputeRefreshOption& other);
  RecomputeRefreshOption& operator=(const RecomputeRefreshOption& other);

  const NABoolean	noDelete_;
};  // class RecomputeRefreshOption

//----------------------------------------------------------------------------
class NRowsClause : public NABasicObject
{
public:
  NRowsClause(Lng32 commitEach, 
	      Lng32 phase,
	      ItemExpr *pOptionalCatchupClause)
  : commitEach_(commitEach),
    phase_(phase),
    pCatchupClause_(pOptionalCatchupClause)
  {}

  // The pCatchupClause_ is not deleted because it is used in therefresh tree.
  virtual ~NRowsClause() {}

  NABoolean isPhase1()   const { return getPhase() == 1; }
  NABoolean isCatchup()  const { return getCatchup() != NULL; }

  Lng32      getCommitEach() const { return commitEach_; }
  Lng32      getPhase()      const { return phase_; }
  ItemExpr *getCatchup()    const { return pCatchupClause_; }

private:
  // Copy Ctor and = operator are not implemented.
  NRowsClause(const NRowsClause& other);
  NRowsClause& operator=(const NRowsClause& other);

  Lng32      commitEach_;
  Lng32      phase_;
  ItemExpr *pCatchupClause_;	
};  // class NRowsClause

//----------------------------------------------------------------------------
// This is an intermediate class, used internally by the parser.
class IncrementalRefreshOption : public NABasicObject
{
public:
  enum refreshType { SINGLEDELTA, MULTIDELTA };
  
  IncrementalRefreshOption(DeltaDefinitionPtrList *pDefList, 
			   NRowsClause            *pOptionalNRowsClause,
			   PipelineClause         *pOptionalPipelineClause)
  : type_(SINGLEDELTA),
    pDeltaDefList_(pDefList),
    pNRowsClause_(pOptionalNRowsClause),
    phaseVal_(0),
    pPipelineClause_(pOptionalPipelineClause)
  {}

  
  IncrementalRefreshOption(DeltaDefinitionPtrList *pDefList, 
			   Lng32                    phaseVal,
			   PipelineClause         *pOptionalPipelineClause)
  : type_(MULTIDELTA),
    pDeltaDefList_(pDefList),
    pNRowsClause_(NULL),
    phaseVal_(phaseVal),
    pPipelineClause_(pOptionalPipelineClause)
  {}

  // Don't delete anything here! All data members are given to the Refresh
  // node as parameters to the Ctor.
  virtual ~IncrementalRefreshOption() {}

  void synthesize()
  {
    for (CollIndex i = 0 ; i < pDeltaDefList_->entries() ; i++)
      ((DeltaDefinition *)pDeltaDefList_->at(i))->synthesize();
  }

  refreshType	          getType()            { return type_; }
  DeltaDefinitionPtrList *getDeltaDefPtrList() { return pDeltaDefList_; } 
  NRowsClause	         *getNRowsClause()     { return pNRowsClause_; }
  Lng32                    getPhaseNum()        { return phaseVal_; }
  PipelineClause         *getPipelineClause()  { return pPipelineClause_; }

public:
  // Copy Ctor and = operator are not implemented.
  IncrementalRefreshOption(const IncrementalRefreshOption& other);
  IncrementalRefreshOption& operator=(const IncrementalRefreshOption& other);

  refreshType		   type_;
  DeltaDefinitionPtrList  *pDeltaDefList_;
  NRowsClause		  *pNRowsClause_;
  Lng32			   phaseVal_;
  PipelineClause	  *pPipelineClause_;
};  // class IncrementalRefreshOption

// ------------------------------------------------------------------------
// Inlines
// ------------------------------------------------------------------------
inline NABoolean DeltaDefinition::isNoDE() const 
{ 
  if (pDeltaOptions_)
	return pDeltaOptions_->getDELevel() == DeltaOptions::NO_DE; 
  else
	return TRUE;
}

inline DeltaOptions::DELevel DeltaDefinition::getDELevel() const 
{
  if (isNoDE())
    return DeltaOptions::NO_DE;
  else
    return pDeltaOptions_->getDELevel();
}

#endif // REFRESH_H

