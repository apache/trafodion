/* -*-C++-*- */
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
//
**********************************************************************/
#ifndef STMTDDLCREATEROUTINE_H
#define STMTDDLCREATEROUTINE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateRoutine.h
 * Description:  class representing Create Routine Statement parser nodes
 *
 *
 * Created:      9/23/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLLocation.h"
#include "ElemDDLParamDefArray.h"
#include "NAString.h"
#include "ParDDLFileAttrsCreateIndex.h"
#include "StmtDDLNode.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// Create Routine statement
// -----------------------------------------------------------------------
class StmtDDLCreateRoutine : public StmtDDLNode
{

public:

  // initialize constructor
  StmtDDLCreateRoutine(const QualifiedName & aRoutineName,
                       const QualifiedName & anActionName,
                       ElemDDLNode         * pParamList,
                       ElemDDLNode         * pReturnsList,
                       ElemDDLNode         * pPassThroughParamList,
                       ElemDDLNode         * pOptionList,
                       ComRoutineType        rType,
                       CollHeap            * heap);

  // virtual destructor
  virtual ~StmtDDLCreateRoutine();

  // cast
  virtual StmtDDLCreateRoutine * castToStmtDDLCreateRoutine();

  //
  // accessors
  //

  // methods relating to parse tree
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 routine);

  inline NABoolean isStmtDDLAlterRoutineParseNode() const;

  inline const NAString getRoutineName() const;
  inline const QualifiedName & getRoutineNameAsQualifiedName() const;
  inline       QualifiedName & getRoutineNameAsQualifiedName() ;

  inline const NAString        getActionNameAsAnsiString() const;
  inline const QualifiedName & getActionNameAsQualifiedName() const;
  inline       QualifiedName & getActionNameAsQualifiedName() ;

        // returns a NAList of ElemDDLParamDef parse nodes.
  inline const ElemDDLParamDefArray & getParamArray() const;
  inline       ElemDDLParamDefArray & getParamArray();

        // returns -1 if there is no RETURNed formal parameter list
  inline const ComSInt32 getFirstReturnedParamPosWithinParamArray() const;
  inline const NABoolean isReturnClauseSpecified() const
                         { return (getFirstReturnedParamPosWithinParamArray() > 0); }

  inline const ElemDDLPassThroughParamDefArray & getPassThroughParamArray() const;
  inline       ElemDDLPassThroughParamDefArray & getPassThroughParamArray();

  inline const ComRoutineType      getRoutineType(void) const;

  inline const ComRoutineLanguage  getLanguageType(void) const;
  inline const NABoolean           isLanguageTypeSpecified() const;
  inline const NABoolean           isDeterministic(void) const;
  inline const ComRoutineSQLAccess getSqlAccess(void) const;
  inline const NABoolean           isCallOnNull(void) const;
  inline const NABoolean           isIsolate(void) const;
  inline const ComRoutineParamStyle getParamStyle(void) const;
  inline const NABoolean           isParamStyleSpecified() const;
  inline const NABoolean           isFinalCall(void) const;
  inline const ComRoutineParallelism getParallelism(void) const { return parallelism_; }
  inline const NABoolean             canBeParallel(void) const;
  inline const ComRoutineTransactionAttributes getTransactionAttributes(void) const;
  inline const ComSInt32            getMaxResults(void) const;
  inline const ComSInt32            getStateAreaSize(void) const;
  inline const NAString           & getUdrAttributes(void) const;
  inline const NAString           & getSpecialAttributes(void) const;
  inline NABoolean                  isSpecialAttributesSpecified() const;
  inline const NAString           & getExternalPath(void) const;
  inline const NAString           & getExternalName(void) const;
  inline const NAString           & getJavaClassName(void) const;
  inline const NAString           & getJavaMethodName(void) const;
  inline const NAString           & getJavaSignature(void) const;
  inline const ComRoutineExternalSecurity getExternalSecurity(void) const { return externalSecurity_; }

  inline NABoolean isLocationSpecified() const;

        // returns an empty string unless the parse node
        // is bound (the method bindNode is invoked).
        // After the parse node is bound, returns the
        // location of the primary partition in Guardian
        // physical device name format.  If the LOCATION
        // clause is not specified, a default location is
        // used.
  inline const NAString & getGuardianLocation() const;
  
  inline const QualifiedName & getLibraryName() const;

        // returns location name if specified; otherwise,
        // an empty string is returned.
  inline const NAString & getLocation() const;

        // returns location name if specified; otherwise,
        // an empty string is returned.
  inline NAString getLocationName() const;

        // returns the type of the specified location name;
        // e.g., a Guardian device name.  If location clause
        // is not specified, the returned value has no meaning.
  ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

  inline const ComSInt32   getParamStyleVersion(void) const { return paramStyleVersion_; }
  inline const ComSInt32   getInitialCpuCost   (void) const { return initialCpuCost_; }
  inline const ComSInt32   getInitialIoCost    (void) const { return initialIoCost_; }
  inline const ComSInt32   getInitialMsgCost   (void) const { return initialMsgCost_; }
  inline const ComSInt32   getNormalCpuCost    (void) const { return normalCpuCost_; }
  inline const ComSInt32   getNormalIoCost     (void) const { return normalIoCost_; }
  inline const ComSInt32   getNormalMsgCost    (void) const { return normalMsgCost_; }
  inline const ComBoolean  getIsUniversal      (void) const { return isUniversal_; }
  inline const ComBoolean  getCanCollapse      (void) const { return canCollapse_; }
  inline const ComString & getUserVersion      (void) const { return userVersion_; }
  inline const ComString & getVersionTag       (void) const { return userVersion_; }
  inline const ComSInt32   getActionPosition   (void) const { return actionPosition_; }
  inline const ComRoutineExecutionMode getExecutionMode (void) const { return executionMode_; }
  inline const NAList<ComSInt64> & getUniqueOutputValues(void) const { return uniqueOutputValues_; }
  inline const NABoolean isOwnerSpecified() const;
  inline const ElemDDLGrantee *getOwner() const;

  inline const NAList<ComUudfParamKind> & getUudfParamKindList (void) const;

  inline ElemDDLNode * getPassThroughInputsParseTree(void);
  inline ElemDDLNode * getRoutineAttributesParseTree(void);

  //
  // mutators
  //

  inline void setRoutineType(ComRoutineType routineType);
  inline void setUudfName(const QualifiedName &uudfQualName);

  // methods relating to Parse trees
  void setChild(Lng32 routine, ExprNode * newNode);
  inline void setUudfParamKindList(ElemDDLNode *pElemDDL);
  inline void setPassThroughInputsParseTree(ElemDDLNode *pElemDDL);
  inline void setRoutineAttributesParseTree(ElemDDLNode *pElemDDL);
  inline void setOwner(ElemDDLNode * pAuthID);

  //
  // method for binding
  //

  ExprNode * bindNode(BindWA * pBindWA);

  //
  // Method for collecting information
  //   Collects information in the parse sub-tree and
  //   copy/move them to the current parse node.
  //
  
  void synthesize(void);

  void setRoutineOption(ElemDDLNode * pRoutineOption);

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1(void) const;
  virtual const NAString displayLabel2(void) const;
  virtual NATraceList getDetailInfo(void) const;
  virtual const NAString getText(void) const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // please do not use the following methods
  //
  
  StmtDDLCreateRoutine();                                          // DO NOT USE
  StmtDDLCreateRoutine(const StmtDDLCreateRoutine &);              // DO NOT USE
  StmtDDLCreateRoutine & operator=(const StmtDDLCreateRoutine &);  // DO NOT USE

protected:

  // ---------------------------------------------------------------------
  // protected data members
  // ---------------------------------------------------------------------

  QualifiedName routineQualName_;
  QualifiedName actionQualName_; // Only used when routineType_ == COM_ACTION_UDF_TYPE

  // list of parameters
  // For UDF, includes the RETURNed formal parameters to the right of the list
  ElemDDLParamDefArray paramArray_;
  ComSInt32 firstReturnedParamPosWithinParamArray_;

  // list of pass through parameters (specified in the definition of a routine);
  // list of new parameters added to the existing ones when appearing within
  // the alteration of a routine (e.g., in an ALTER FUNCTION statement).
  ElemDDLPassThroughParamDefArray passThroughParamArray_;

  ComRoutineType routineType_;
  
  //
  // LOCATION clause
  //
  NABoolean isLocationClauseSpec_;
  NAString locationName_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;

  // guardianLocation_ is empty until the parse node is
  // bound (the method bindNode() is invoked).  The method
  // bindNode() converts the specified location (in the
  // LOCATION clause associating with the primary key) to
  // a Guardian physical device and then saves the computed
  // name in this data member.  If the LOCATION clause does
  // not appear, a default location name is selected.

  NAString guardianLocation_;

  // LANGUAGE clause
  ComRoutineLanguage   languageType_;
  NABoolean            languageTypeSpecified_;

  NABoolean            deterministic_;
  NABoolean            deterministicSpecified_;

  ComRoutineSQLAccess  sqlAccess_;
  NABoolean            sqlAccessSpecified_;

  NABoolean            callOnNull_;
  NABoolean            callOnNullSpecified_;

  NABoolean            isolate_;
  NABoolean            isolateSpecified_;

  ComRoutineParamStyle paramStyle_;
  NABoolean            paramStyleSpecified_;

  ComRoutineTransactionAttributes transactionAttributes_;
  NABoolean            transactionAttributesSpecified_;

  ComSInt32            maxResults_;
  NABoolean            maxResultSetsSpecified_;

  ComSInt32            stateAreaSize_;
  NABoolean            stateAreaSizeSpecified_;

  NAString             udrAttributes_;
  NABoolean            udrAttributesSpecified_;

  NAString             externalPath_;
  NABoolean            externalPathSpecified_;

  NAString             externalFile_;

  NAString             externalName_;
  NABoolean            externalNameSpecified_;

  NAString             javaClassName_;
  NAString             javaMethodName_;

  NAString             javaSignature_;
  
  QualifiedName        libraryName_;
  NABoolean            libraryNameSpecified_;

  NABoolean            finalCall_;
  NABoolean            finalCallSpecified_;

  ComRoutineParallelism parallelism_;
  NABoolean            parallelismSpecified_;

  ComSInt32            paramStyleVersion_;
  NABoolean            paramStyleVersionSpecified_;

  ComSInt32            initialCpuCost_;
  NABoolean            initialCpuCostSpecified_;

  ComSInt32            initialIoCost_;
  NABoolean            initialIoCostSpecified_;

  ComSInt32            initialMsgCost_;
  NABoolean            initialMsgCostSpecified_;

  ComSInt32            normalCpuCost_;
  NABoolean            normalCpuCostSpecified_;

  ComSInt32            normalIoCost_;
  NABoolean            normalIoCostSpecified_;

  ComSInt32            normalMsgCost_;
  NABoolean            normalMsgCostSpecified_;

  ComBoolean           isUniversal_;
  NABoolean            isUniversalSpecified_;

  ComBoolean           canCollapse_;
  NABoolean            canCollapseSpecified_;

  ComString            userVersion_;
  NABoolean            userVersionSpecified_;

  ComRoutineExternalSecurity externalSecurity_;
  NABoolean            externalSecuritySpecified_;

  ComSInt32            actionPosition_;
  NABoolean            actionPositionSpecified_;

  ComRoutineExecutionMode executionMode_;
  NABoolean            executionModeSpecified_;

  NAList<ComUudfParamKind> uudfParamKindList_;
  NABoolean            uudfParamKindListSpecified_;

  NAList<ComSInt64>    uniqueOutputValues_;
  NABoolean            numberOfUniqueOutputValuesSpecified_;

  NAString             specialAttributesText_;
  NABoolean            specialAttributesSpecified_;

  ElemDDLGrantee      *pOwner_;

  //
  // pointers to child parse nodes
  //

  enum { INDEX_ROUTINE_PARAM_LIST = 0,
         INDEX_ROUTINE_OPTION_LIST,
         INDEX_ROUTINE_RETURNS_LIST,
         INDEX_ROUTINE_PASSTHROUGH_LIST,
         INDEX_ROUTINE_UUDF_PARAM_KIND_LIST,
         MAX_STMT_DDL_CREATE_ROUTINE_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_CREATE_ROUTINE_ARITY];

}; // class StmtDDLCreateRoutine

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateRoutine
// -----------------------------------------------------------------------

inline QualifiedName &
StmtDDLCreateRoutine::getRoutineNameAsQualifiedName()
{
  return routineQualName_;
}

inline const QualifiedName & 
StmtDDLCreateRoutine::getRoutineNameAsQualifiedName() const
{
  return routineQualName_;
}

// get routine name
inline const NAString
StmtDDLCreateRoutine::getRoutineName() const
{
  return routineQualName_.getQualifiedNameAsAnsiString();
}

// get routine action name
inline const NAString
StmtDDLCreateRoutine::getActionNameAsAnsiString() const
{
  return actionQualName_.getQualifiedNameAsAnsiString();
}

inline QualifiedName &
StmtDDLCreateRoutine::getActionNameAsQualifiedName()
{
  return actionQualName_;
}

inline const QualifiedName &
StmtDDLCreateRoutine::getActionNameAsQualifiedName() const
{
  return actionQualName_;
}

inline const ElemDDLParamDefArray &
StmtDDLCreateRoutine::getParamArray() const
{
  return paramArray_;
}

inline ElemDDLParamDefArray &
StmtDDLCreateRoutine::getParamArray()
{
  return paramArray_;
}

inline const ComSInt32
StmtDDLCreateRoutine::getFirstReturnedParamPosWithinParamArray() const
{
  return firstReturnedParamPosWithinParamArray_;
}

inline const ElemDDLPassThroughParamDefArray &
StmtDDLCreateRoutine::getPassThroughParamArray() const
{
  return passThroughParamArray_;
}

inline ElemDDLPassThroughParamDefArray &
StmtDDLCreateRoutine::getPassThroughParamArray()
{
  return passThroughParamArray_;
}

inline const QualifiedName &
StmtDDLCreateRoutine::getLibraryName() const
{
  return libraryName_;
}

inline const NAString &
StmtDDLCreateRoutine::getLocation() const
{
  return locationName_;
}

inline NAString
StmtDDLCreateRoutine::getLocationName() const
{
  return locationName_;
}

inline ElemDDLLocation::locationNameTypeEnum
StmtDDLCreateRoutine::getLocationNameType() const
{
  return locationNameType_;
}

// is location clause specified?
inline NABoolean
StmtDDLCreateRoutine::isLocationSpecified() const
{
  return isLocationClauseSpec_;
}

inline const ComRoutineType
StmtDDLCreateRoutine::getRoutineType(void) const
{
  return routineType_;
}

inline const ComRoutineLanguage
StmtDDLCreateRoutine::getLanguageType(void) const
{
  return languageType_;
}

inline const NABoolean
StmtDDLCreateRoutine::isLanguageTypeSpecified() const
{
  return languageTypeSpecified_;
}

inline const NABoolean
StmtDDLCreateRoutine::isDeterministic(void) const
{
  return deterministic_;
}

inline const ComRoutineSQLAccess
StmtDDLCreateRoutine::getSqlAccess(void) const
{
  return sqlAccess_;
}

inline const NABoolean
StmtDDLCreateRoutine::isCallOnNull(void) const
{
  return callOnNull_;
}

inline const NABoolean
StmtDDLCreateRoutine::isIsolate(void) const
{
  return isolate_;
}

inline const ComRoutineParamStyle
StmtDDLCreateRoutine::getParamStyle(void) const
{
  return paramStyle_;
}

inline const NABoolean
StmtDDLCreateRoutine::isParamStyleSpecified() const
{
  return paramStyleSpecified_;
}

inline const ComRoutineTransactionAttributes
StmtDDLCreateRoutine::getTransactionAttributes(void) const
{
  return transactionAttributes_;
}

inline const ComSInt32
StmtDDLCreateRoutine::getMaxResults(void) const
{
  return maxResults_;
}

inline const ComSInt32
StmtDDLCreateRoutine::getStateAreaSize(void) const
{
  return stateAreaSize_;
}

inline const NAString &
StmtDDLCreateRoutine::getUdrAttributes(void) const
{
  return udrAttributes_;
}

inline const NAString &
StmtDDLCreateRoutine::getExternalPath(void) const
{
  return externalPath_;
}

inline const NAString &
StmtDDLCreateRoutine::getExternalName(void) const
{
  return externalName_;
}

inline const NAString &
StmtDDLCreateRoutine::getJavaClassName(void) const
{
  return javaClassName_;
}

inline const NAString &
StmtDDLCreateRoutine::getJavaMethodName(void) const
{
  return javaMethodName_;
}

inline const NAString &
StmtDDLCreateRoutine::getJavaSignature(void) const
{
  return javaSignature_;
}

inline const NABoolean
StmtDDLCreateRoutine::isFinalCall(void) const
{
  return finalCall_;
}

inline const NABoolean
StmtDDLCreateRoutine::canBeParallel(void) const
{
  return (parallelism_ NEQ COM_ROUTINE_NO_PARALLELISM);
}

inline const NAList<ComUudfParamKind> &
StmtDDLCreateRoutine::getUudfParamKindList (void) const
{
  return uudfParamKindList_;
}

inline ElemDDLNode *
StmtDDLCreateRoutine::getPassThroughInputsParseTree()
{
  if (getChild(INDEX_ROUTINE_PASSTHROUGH_LIST) NEQ NULL)
    return getChild(INDEX_ROUTINE_PASSTHROUGH_LIST)->castToElemDDLNode();
  else
    return NULL;
}

inline ElemDDLNode *
StmtDDLCreateRoutine::getRoutineAttributesParseTree()
{
  if (getChild(INDEX_ROUTINE_OPTION_LIST) NEQ NULL)
    return getChild(INDEX_ROUTINE_OPTION_LIST)->castToElemDDLNode();
  else
    return NULL;
}

inline NABoolean
StmtDDLCreateRoutine::isStmtDDLAlterRoutineParseNode() const
{
  return (const_cast<StmtDDLCreateRoutine *>(this)
          ->castToStmtDDLAlterRoutine() NEQ NULL);
}

inline const NAString &
StmtDDLCreateRoutine::getSpecialAttributes(void) const
{
  return specialAttributesText_;
}

inline NABoolean
StmtDDLCreateRoutine::isSpecialAttributesSpecified() const
{
  return specialAttributesSpecified_;
}

//
// inline mutators
//

inline void
StmtDDLCreateRoutine::setRoutineType(ComRoutineType routineType)
{
  routineType_ = routineType;
}

inline void
StmtDDLCreateRoutine::setUudfName(const QualifiedName &uudfQualName)
{
  routineQualName_ = uudfQualName;
}

inline void
StmtDDLCreateRoutine::setUudfParamKindList(ElemDDLNode *pElemDDL)
{
  setChild(INDEX_ROUTINE_UUDF_PARAM_KIND_LIST, pElemDDL);
}

inline void
StmtDDLCreateRoutine::setPassThroughInputsParseTree(ElemDDLNode *pElemDDL)
{
  setChild(INDEX_ROUTINE_PASSTHROUGH_LIST, pElemDDL);
}

inline void
StmtDDLCreateRoutine::setRoutineAttributesParseTree(ElemDDLNode *pElemDDL)
{
  setChild(INDEX_ROUTINE_OPTION_LIST, pElemDDL);
}

inline const NABoolean
StmtDDLCreateRoutine::isOwnerSpecified() const
{
  return pOwner_ ? TRUE : FALSE;
}

inline const ElemDDLGrantee *
StmtDDLCreateRoutine::getOwner() const
{
  return pOwner_;
}

inline void StmtDDLCreateRoutine::setOwner(ElemDDLNode * pAuthID)
{
  if (pAuthID)
  {
    ComASSERT(pAuthID->castToElemDDLGrantee());
    pOwner_ = pAuthID->castToElemDDLGrantee();
  }
  else
    pOwner_ = NULL;
}

#endif // STMTDDLCREATEROUTINE_H
