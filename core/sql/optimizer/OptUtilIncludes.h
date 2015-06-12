/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
#ifndef OPTUTILINCLUDES_H
#define OPTUTILINCLUDES_H
/* -*-C++-*-
*************************************************************************
*
* File:         OptUtilIncludes.h
* Description:  Contains general utility classes, structs and other stuff
*               used by binder, optimizer, generator and other components.
*               This class is NOT stored on disk or used by executor.
* Created:      8/1/2000
* Language:     C++
*
*
*************************************************************************
*/

#include "ObjectNames.h"
#include "SqlTableOpenInfo.h"

class OptSqlTableOpenInfo : public NABasicObject
{
public:
  OptSqlTableOpenInfo(SqlTableOpenInfo * stoi, 
                      const CorrName &corrName,
                      NAMemory* h)
       : stoi_(stoi), 
       corrName_(corrName,h), 
       table_(NULL),
       insertColList_(h),
       updateColList_(h),
       selectColList_(h)
  {}

  SqlTableOpenInfo * getStoi() { return stoi_; }
  const CorrName &getCorrName() { return corrName_; }
  void setTable(NATable* ta) {table_ = ta;}
  NATable* getTable() {return table_;}
  const LIST (Lng32) &	getInsertColList() const { return insertColList_; }
  const LIST (Lng32) &	getSelectColList() const { return selectColList_; }
  const LIST (Lng32) &	getUpdateColList() const { return updateColList_; }

  void addInsertColumn(Lng32 col) 
    { if (!insertColList_.contains(col)) insertColList_.insert(col); }
  void addUpdateColumn(Lng32 col) 
    { if (!updateColList_.contains(col)) updateColList_.insert(col); }
  void addSelectColumn(Lng32 col) 
    { if (!selectColList_.contains(col)) selectColList_.insert(col); }
  bool checkColPriv(const PrivType privType);
  
private:
  SqlTableOpenInfo * stoi_;
  CorrName corrName_;
  NATable * table_;
  LIST(Lng32) insertColList_ ;
  LIST(Lng32) selectColList_ ;
  LIST(Lng32) updateColList_ ;
};

class OltOptInfo : public NABasicObject
{
public:
  OltOptInfo()
       : flags_(0)
  {}

  NABoolean oltAllOpt() { return (oltCliOpt() && oltMsgOpt() && oltEidOpt());}
  NABoolean oltAnyOpt() { return (oltCliOpt() || oltMsgOpt() || oltEidOpt());}

  NABoolean oltCliOpt() { return (flags_ & OLT_CLI_OPT) != 0; }
  NABoolean oltMsgOpt() { return (flags_ & OLT_MSG_OPT) != 0; }
  NABoolean oltEidOpt() { return (flags_ & OLT_EID_OPT) != 0; }
  NABoolean oltEidLeanOpt() { return (flags_ & OLT_EID_LEAN_OPT) != 0; }

  NABoolean multipleRowsReturned() { return (flags_ & MULTI_ROWS_RETURNED) != 0; }

  NABoolean maxOneRowReturned() { return (flags_ & MAX_ONE_ROW_RETURNED) != 0; }

  NABoolean maxOneInputRow() { return (flags_ & MAX_ONE_INPUT_ROW) != 0; }

  NABoolean disableOperStats() { return (flags_ & DISABLE_OPERATOR_STATS) != 0; }

  void setOltOpt(NABoolean v)      
  {
    setOltCliOpt(v);
    setOltMsgOpt(v);
    setOltEidOpt(v);
  }

  void setOltCliOpt(NABoolean v)      
           { (v ? flags_ |= OLT_CLI_OPT : flags_ &= ~OLT_CLI_OPT); }
  void setOltMsgOpt(NABoolean v)      
           { (v ? flags_ |= OLT_MSG_OPT : flags_ &= ~OLT_MSG_OPT); }
  void setOltEidOpt(NABoolean v)      
           { (v ? flags_ |= OLT_EID_OPT : flags_ &= ~OLT_EID_OPT); }
  void setOltEidLeanOpt(NABoolean v)      
           { (v ? flags_ |= OLT_EID_LEAN_OPT : flags_ &= ~OLT_EID_LEAN_OPT); }
  void setMultipleRowsReturned(NABoolean v)      
           { (v ? flags_ |= MULTI_ROWS_RETURNED : flags_ &= ~MULTI_ROWS_RETURNED); }
  void setMaxOneRowReturned(NABoolean v)      
           { (v ? flags_ |= MAX_ONE_ROW_RETURNED : flags_ &= ~MAX_ONE_ROW_RETURNED); }
  void setMaxOneInputRow(NABoolean v)      
           { (v ? flags_ |= MAX_ONE_INPUT_ROW : flags_ &= ~MAX_ONE_INPUT_ROW); }
  void mayDisableOperStats(OltOptInfo *oltOptInfo)
  {
    if (oltOptInfo->oltMsgOpt() || oltOptInfo->oltEidOpt() || oltOptInfo->oltEidLeanOpt())
    {
      flags_ |= DISABLE_OPERATOR_STATS;
    }
  }
private:
  
  enum Flags
  {
    // set to true if olt opt could be done at cli level
    OLT_CLI_OPT = 0x0001,
    
    // set to true if olt opt could be done for PA/EID messages
    OLT_MSG_OPT = 0x0002,
    
    // set to true if olt opt could be done for calls to DP2 in EID.
    OLT_EID_OPT = 0x0004,

    // set if multiple rows are returned from a leaf operator.
    MULTI_ROWS_RETURNED = 0x0008,

    // set if LEAN (less space consumption and faster execution)
    // olt opt could be done for EID leaf operators.
    OLT_EID_LEAN_OPT = 0x0010,

    // set if a dp2 leaf non-unique operator will return max one row.
    // Set for update/delete subset which don't return rows, or
    // if first 1 is used with a scan operator. 
    // Used by PA operator to allocate small buffers.
    MAX_ONE_ROW_RETURNED = 0x0020,

    MAX_ONE_INPUT_ROW = 0x0040,

    // This flag will be set whenever OLT_MSG_OPT, OLD_EID_OPT or OLT_EID_LEAN_OPT
    // is set to true in the individual RelExpr OltOptInfo
    DISABLE_OPERATOR_STATS = 0x0080
  };

  ULng32 flags_;

};

class OptUdrOpenInfo : public NABasicObject
{
public:
  OptUdrOpenInfo(SqlTableOpenInfo * stoi, const NAString &name, NARoutine *pNAR )
       : udrStoi_(stoi), udrName_(name), 
         NARoutine_(pNAR)
  {}

  SqlTableOpenInfo * getUdrStoi() { return udrStoi_; }
  const NAString &getUdrName() { return udrName_; }
  NARoutine * getNARoutine() { return NARoutine_; }

  void setUdrStoi( SqlTableOpenInfo *stoi )
  {
    udrStoi_ = stoi;
  }

private:
  SqlTableOpenInfo *udrStoi_;
  const NAString    udrName_;
  NARoutine      *NARoutine_;
};
 
class OptUDFInfo : public NABasicObject
{
public:
  OptUDFInfo(Int64 udfUID,
             const ComObjectName& udfName,
	     CollHeap *heap = CmpCommon::statementHeap())
       : udfUID_(udfUID),
         udfName_(udfName),
         heap_(heap),
         nameSpace_(COM_UDF_NAME)
  {}

  OptUDFInfo(Int64 udfUID,
             const ComObjectName& udfName,
             const ComAnsiNameSpace& ansiNameSpace,
             const ComObjectName& internalObjectNameForAction,
	     CollHeap *heap = CmpCommon::statementHeap())
       : udfUID_(udfUID),
         udfName_(udfName),
         heap_(heap),
         nameSpace_(ansiNameSpace),
	 internalObjectNameForAction_(internalObjectNameForAction)
  {}
  
  ~OptUDFInfo(){ };

  // Accessors

  inline const ComObjectName& getUDFName() const          { return udfName_; } 
  inline const NAString getUDFExternalName() const        { return udfName_.getExternalName(); } 
  inline const ComAnsiNameSpace getUDFNameSpace() const   { return nameSpace_; }
  inline const Int64 getUDFUID() const          { return udfUID_; } 

  inline const ComObjectName& getInternalObjectNameForAction() const
    { return internalObjectNameForAction_; }

  inline const NAString getInternalObjectNameForActionExternalName() const
    { return internalObjectNameForAction_.getExternalName(); } 

  inline CollHeap * getHeapPtr()  { return heap_; }

  // Settors
  inline void setInternalObjectNameForAction(ComObjectName & internalObjectName )
  { internalObjectNameForAction_ = internalObjectName; } 

  inline void setUDFNameSpace(ComAnsiNameSpace nameSpace)
  { nameSpace_ = nameSpace; }

private:
   
  Int64 udfUID_;
  ComObjectName udfName_;
  ComAnsiNameSpace nameSpace_;
  ComObjectName internalObjectNameForAction_;
  CollHeap *heap_;
};
 
#endif

